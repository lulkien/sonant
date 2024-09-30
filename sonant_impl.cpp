#include "sonant_impl.h"
#include "sonant.h"
#include "sonant_utils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

#include <alsa/error.h>
#include <alsa/pcm.h>

// #define DEBUG_MODE
#define ENABLE_WHISPER
#define BUFFER_REVERSE_SIZE  102400

SonantImpl::SonantImpl() {
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::steady_clock;

    // Reserved memory for record buffer
    m_recordBuffer.reserve(BUFFER_REVERSE_SIZE);

    // Just set init time value at 100s in the past
    m_lastInputTime =
        steady_clock::now() - std::chrono::seconds(100);
}

SonantImpl::~SonantImpl() {
    stopRecorder();

    if (nullptr != m_whisperCtx) {
        whisper_free(m_whisperCtx);
        m_whisperCtx = nullptr;
    }

    terminate();
}

bool SonantImpl::initialize(const std::string &modelPath,
                            const SonantParams &params) {
    // Init ASLS
    int alsaError;
    snd_pcm_hw_params_t *hwParams;
    unsigned int sample_rate = SONANT_RECORD_FREQ;
    unsigned int channels = SONANT_RECORD_CHANNELS;

    if ((alsaError = snd_pcm_open(&m_alsaCapture, "default",
                                  SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        ERR_LOG << "Cannot open audio device: " << snd_strerror(alsaError)
                << LOG_ENDL;
        return false;
    }

    snd_pcm_hw_params_malloc(&hwParams);

    snd_pcm_hw_params_any(m_alsaCapture, hwParams);
    snd_pcm_hw_params_set_access(m_alsaCapture, hwParams,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(m_alsaCapture, hwParams, SONANT_RECORD_FORMAT);
    snd_pcm_hw_params_set_rate_near(m_alsaCapture, hwParams, &sample_rate,
                                    nullptr);
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
    whisper_context_params model_params = whisper_context_default_params();
    m_whisperCtx =
        whisper_init_from_file_with_params(modelPath.c_str(), model_params);

    // Config parameters for whisper
    m_whisperParams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    m_whisperParams.n_threads = params.sonantThreadCount;
    m_whisperParams.print_special = false; // Don't print special tokens like <SOT>, <EOT>, etc.
    m_whisperParams.print_progress = false; // Don't print progress information.
    m_whisperParams.single_segment = true;  // Return transcript as one string.

    if (nullptr == m_whisperCtx) {
        ERR_LOG << "Failed to init whisper context from model: " << modelPath
                << LOG_ENDL;
        return false;
    } else {
        m_whisperModelPath = modelPath;
        m_whisperInitOk = true;
    }
#endif // !ENABLE_WHISPER

    return true;
}

bool SonantImpl::requestChangeModel(const std::string &newModelPath) {
    if (newModelPath == m_whisperModelPath) {
        return true;
    }

    return reloadModel(newModelPath);
}

bool SonantImpl::startRecorder() {
#ifdef ENABLE_WHISPER
    if (m_whisperModelPath.empty()) {
        ERR_LOG << "Model path was not set." << LOG_ENDL;
        return false;
    }
#endif

    if (!m_alsaInitOk) {
        ERR_LOG << "ASLA was init correctly." << LOG_ENDL;
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

void SonantImpl::setCallbackTranscriptionReady(
    std::function<void(std::string)> callback) {
    m_callbackTranscriptionReady = callback;
}

void SonantImpl::doRecordAudio() {
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    // Create a buffer for recording
    std::array<float, SONANT_RECORD_SAMPLE_PER_FRAME> recorded_frame;
    while (true) {
        if (!m_alsaInitOk) {
            UNREACHABLE();
            break;
        }

        if (m_terminate.load()) {
#ifdef DEBUG_MODE
            MSG_LOG << "Terminate record thread" << LOG_ENDL;
#endif // DEBUG_MODE
            break;
        }

        if (!m_recording.load()) {
            sleep_for(milliseconds(100));
            continue;
        }

        alsaCaptureHandle(recorded_frame);
    }

#ifdef DEBUG_MODE
    MSG_LOG << "Record worker END." << LOG_ENDL;
#endif // DEBUG_MODE
}

void SonantImpl::alsaCaptureHandle(std::array<float, SONANT_RECORD_SAMPLE_PER_FRAME>& frame) {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::chrono::steady_clock;
    using std::this_thread::sleep_for;

    steady_clock::time_point now = steady_clock::now();
    milliseconds deltaTimeSinceLastInput =
        duration_cast<milliseconds>(now - m_lastInputTime);

    // Clear data before record
    frame.fill(0.0f);

    // Read data from source and save to buffer
    int byte_len = snd_pcm_readi(m_alsaCapture, frame.data(),
                                 SONANT_RECORD_SAMPLE_PER_FRAME);
    if (byte_len < 0) {
        byte_len = snd_pcm_recover(m_alsaCapture, byte_len, 0);
    }
    if (byte_len < 0) {
        ERR_LOG << "Failed to read PCM data: " << strerror(byte_len) << LOG_ENDL;
        return;
    }

    // Check if buffer have item above
    auto it = std::find_if(
                  frame.begin(), frame.begin() + byte_len,
    [this](float sample) {
        return abs(sample) > m_recordThreshold;
    });

    bool has_input = (it != frame.begin() + byte_len);

    if (has_input || deltaTimeSinceLastInput < m_stopRecordDelay) {
        if (has_input) {
            m_lastInputTime = now;
        }

        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_recordBuffer.insert(m_recordBuffer.end(), frame.begin(),
                              frame.begin() + byte_len);
    } else {
        processRecordBuffer();
    }
}

void SonantImpl::processRecordBuffer() {
    std::unique_lock<std::mutex> record_buffer_lock(m_bufferMutex,
            std::defer_lock);
    std::unique_lock<std::mutex> whisper_context_lock(m_whisperMutex,
            std::defer_lock);

    if (!record_buffer_lock.try_lock()) {
        ERR_LOG << "Failed to acquire buffer lock. Process buffer operation "
                "aborted."
                << LOG_ENDL;
        return;
    }

    if (m_recordBuffer.empty()) {
        return;
    }

    if (!m_callbackTranscriptionReady) {
        // callback not set, just clear the buffer and listen to new data
        m_recordBuffer.clear();
        return;
    }

#ifdef DEBUG_MODE
    MSG_LOG << "Pass buffer to whisper. Buffer size = " << m_recordBuffer.size()
            << LOG_ENDL;
#endif // DEBUG_MODE

#ifdef ENABLE_WHISPER
    do {
        if (!whisper_context_lock.try_lock()) {
            ERR_LOG << "Failed to acquire whisper context lock. Process buffer "
                    "operation aborted."
                    << LOG_ENDL;
            return;
        }

        int result = whisper_full(m_whisperCtx, m_whisperParams,
                                  m_recordBuffer.data(), m_recordBuffer.size());

        m_recordBuffer.clear();

        if (result != 0) {
            ERR_LOG << "Failed to transcript audio." << LOG_ENDL;
            break;
        }

        int segmentCount = whisper_full_n_segments(m_whisperCtx);
        std::string transcript = "[UNDETECTED]";
        if (segmentCount > 0) {
            transcript = whisper_full_get_segment_text(m_whisperCtx, 0);
        }

        // Checked functor above already
        m_callbackTranscriptionReady(transcript);
    } while (false);

#endif // ENABLE_WHISPER

#ifdef DEBUG_MODE
    MSG_LOG << "END" << LOG_ENDL;
#endif // DEBUG_MODE
}

bool SonantImpl::reloadModel(const std::string &newModelPath) {
    std::unique_lock<std::mutex> whisper_context_lock(m_whisperMutex);

    if (!whisper_context_lock.try_lock()) {
        ERR_LOG << "Failed to acquire whisper context lock. Reload model "
                "operation aborted."
                << LOG_ENDL;
        return false;
    }

    whisper_context *old_whisper_context = m_whisperCtx;

    whisper_context_params model_params = whisper_context_default_params();
    m_whisperCtx =
        whisper_init_from_file_with_params(newModelPath.c_str(), model_params);

    if (nullptr == m_whisperCtx) {
        ERR_LOG << "Failed to create whisper context from new model: "
                << newModelPath << ". Fallback to old context." << LOG_ENDL;
        m_whisperCtx = old_whisper_context;
        return false;
    }

    m_whisperModelPath = newModelPath;
    whisper_free(old_whisper_context);
    return true;
}
