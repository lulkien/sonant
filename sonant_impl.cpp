#include "sonant_impl.h"
#include "sonant.h"
#include "sonant_utils.h"
#include <algorithm>
#include <alsa/error.h>
#include <alsa/pcm.h>
#include <cmath>
#include <iostream>

#define ENABLE_WHISPER
// #define DEBUG_MODE

#define SONANT_RECORD_FREQ              16000 // Whisper use 16kHz
#define SONANT_RECORD_FORMAT            SND_PCM_FORMAT_FLOAT_LE
#define SONANT_RECORD_CHANNELS          1
#define SONANT_RECORD_SAMPLE_PER_FRAME  256

SonantImpl::SonantImpl() {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;

    // Convert 00:00:00 1/1/1970 to steady_clock::time_point
    m_lastInputTime = steady_clock::now() + duration_cast<steady_clock::duration>(seconds(0));
}

SonantImpl::~SonantImpl() {
    stopRecorder();

    if (nullptr != m_whisperCtx) {
        whisper_free(m_whisperCtx);
        m_whisperCtx = nullptr;
    }

    terminate();
}

bool SonantImpl::initialize(const std::string& modelPath, const SonantParams& params) {
    // Init ASLS
    int alsaError;
    snd_pcm_hw_params_t *hwParams;
    unsigned int sample_rate = SONANT_RECORD_FREQ;
    unsigned int channels = SONANT_RECORD_CHANNELS;

    if ((alsaError = snd_pcm_open(&m_alsaCapture, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0 ) {
        ERR_LOG << "Cannot open audio device: " << snd_strerror(alsaError) << std::endl;
        return false;
    }

    snd_pcm_hw_params_malloc(&hwParams);

    snd_pcm_hw_params_any(m_alsaCapture, hwParams);
    snd_pcm_hw_params_set_access(m_alsaCapture, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(m_alsaCapture, hwParams, SONANT_RECORD_FORMAT);
    snd_pcm_hw_params_set_rate_near(m_alsaCapture, hwParams, &sample_rate, nullptr);
    snd_pcm_hw_params_set_channels(m_alsaCapture, hwParams, channels);
    snd_pcm_hw_params(m_alsaCapture, hwParams);

    snd_pcm_hw_params_free(hwParams);

    // Init recorder
    m_recorderThread = std::thread(&SonantImpl::doRecordAudio, this);
    m_recordThreshold = params.recordThreshold;
    m_stopRecordDelay = std::chrono::milliseconds(params.pauseRecorderDelay);

    // ALSA init OK
    m_alsaInitOk = true;

#ifdef ENABLE_WHISPER
    // Init whisper
    struct whisper_context_params model_params = whisper_context_default_params();
    m_whisperCtx = whisper_init_from_file_with_params(modelPath.c_str(), model_params);

    // Config parameters for whisper
    m_whisperParams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    m_whisperParams.n_threads = params.sonantThreadCount;
    m_whisperParams.print_special = false; // Print special tokens like <SOT>, <EOT>, etc.
    m_whisperParams.print_progress = false; // Print progress information
    m_whisperParams.single_segment = true;

    if (nullptr == m_whisperCtx) {
        ERR_LOG << "Failed to init whisper context from model: " << modelPath << std::endl;
        return false;
    } else {
        m_whisperModelPath = modelPath;
        m_whisperInitOk = true;
    }
#endif // !ENABLE_WHISPER

    return true;
}

bool SonantImpl::requestChangeModel(const std::string& modelPath) {
    if (modelPath != m_whisperModelPath) {
        if (reloadModel(modelPath)) {
            m_whisperModelPath = modelPath;
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool SonantImpl::startRecorder() {
#ifdef ENABLE_WHISPER
    if (m_whisperModelPath.empty()) {
        ERR_LOG << "Model path was not set." << std::endl;
        return false;
    }
#endif

    if (!m_alsaInitOk) {
        ERR_LOG << "ASLA was init correctly." << std::endl;
        return false;
    }

    m_recording.store(true);
    return true;
}

void SonantImpl::stopRecorder() {
    m_recording.store(false);
}

void SonantImpl::terminate() {
    if (!m_terminate.load()) {
        m_terminate.store(true);
    }

    if (m_recorderThread.joinable()) {
        m_recorderThread.join();
    }
}

void SonantImpl::setCallbackTranscriptionReady(std::function<void(std::string)> callback) {
    m_callbackTranscriptionReady = callback;
}

void SonantImpl::doRecordAudio() {
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    // Create tmp buffer for recording
    std::vector<float> tmp_record_buffer(SONANT_RECORD_SAMPLE_PER_FRAME);
    while (true) {
        if (!m_alsaInitOk) {
            UNREACHABLE();
            break;
        }

        if (m_terminate.load()) {
#ifdef DEBUG_MODE
            MSG_LOG << "Terminate record thread" << std::endl;
#endif // DEBUG_MODE
            break;
        }

        if (!m_recording.load()) {
            sleep_for(milliseconds(100));
            continue;
        }

        alsaCaptureHandle(tmp_record_buffer);
    }

#ifdef DEBUG_MODE
    MSG_LOG << "Record worker END." << std::endl;
#endif // DEBUG_MODE
}

void SonantImpl::alsaCaptureHandle(std::vector<float>& buffer) {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    steady_clock::time_point now = steady_clock::now();
    milliseconds deltaTimeSinceLastInput = duration_cast<milliseconds>(now - m_lastInputTime);

    int frames = snd_pcm_readi(m_alsaCapture, buffer.data(), SONANT_RECORD_SAMPLE_PER_FRAME);
    if (frames < 0) {
        frames = snd_pcm_recover(m_alsaCapture, frames, 0);
    }
    if (frames < 0) {
        ERR_LOG << "Failed to read PCM data: " << strerror(frames) << std::endl;
        return;
    }

    auto it = std::find_if(
                  buffer.begin(),
                  buffer.begin() + frames,
    [this](float sample) {
        return abs(sample) > m_recordThreshold;
    });

    bool has_input = (it != buffer.begin() + frames);

    if (has_input || deltaTimeSinceLastInput < m_stopRecordDelay) {
        if (has_input) {
            m_lastInputTime = now;
        }
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_recordBuffer.insert(
                          m_recordBuffer.end(),
                          buffer.begin(),
                          buffer.begin() + frames);
    } else {
        processRecordBuffer();
    }

}

void SonantImpl::processRecordBuffer() {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    if (m_recordBuffer.empty()) {
        return;
    }

#ifdef DEBUG_MODE
    MSG_LOG << "Pass buffer to whisper. Buffer size = "
            << m_recordBuffer.size()
            << std::endl;
#endif // DEBUG_MODE

#ifdef ENABLE_WHISPER
    do {
        m_whisperProcessing.store(true);

        int result = whisper_full(
                         m_whisperCtx,
                         m_whisperParams,
                         m_recordBuffer.data(),
                         m_recordBuffer.size());

        m_recordBuffer.clear();

        if (result != 0) {
            ERR_LOG << "Failed to transcript audio" << std::endl;
            break;
        }

        int segmentCount = whisper_full_n_segments(m_whisperCtx);
        std::string transcript = "[UNDETECTED]";
        if (segmentCount > 0) {
            transcript = whisper_full_get_segment_text(m_whisperCtx, 0);
        }

        m_callbackTranscriptionReady(transcript);

        m_whisperProcessing.store(false);
    } while (false);

#endif // ENABLE_WHISPER


#ifdef DEBUG_MODE
    MSG_LOG << "END" << std::endl;
#endif // DEBUG_MODE
}

bool SonantImpl::reloadModel(const std::string& newModelPath) {
    // TODO: Implement reload model logic
    return true;
}
