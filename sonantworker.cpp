#include "sonantworker.h"
#include "common.h"
#include <QDebug>
#include <sndfile.h>

#define RECORD_SAMPLE_RATE      16000
#define RECORD_CHANNELS         1
#define RECORD_SAMPLE_FORMAT    AUDIO_S16LSB
#define RECORD_BUFFER_SIZE      81920

#define DEFAULT_WHISPER_MODEL   "models/ggml-tiny.en.bin"
#define MAX_SINT16_TO_FLOAT     32768.0f
#define PROCESS_COUNT           6

void audio_callback(void* userdata, Uint8* stream, int len)
{
    Q_UNUSED(stream)
    Q_UNUSED(len)
    if (userdata == nullptr) {
        ERR_LOG << "userdata is nullptr, no where to write";
        return;
    }

    SonantWorker *worker = static_cast<SonantWorker *>(userdata);

    // Ensure that the audio buffer has enough space
    Uint32 newSamplesLength = (len / sizeof(Sint16));
    Uint32 newBufferSize = worker->recordedSampleCount + newSamplesLength + 1;

    if (newBufferSize > worker->recordBufferSize) {
        Sint16 *newRecordBuffer = new Sint16[newBufferSize]{0};
        if (newRecordBuffer == nullptr) {
            ERR_LOG << "Fail to allocate new buffer";
            return;
        }

        if (worker->recordedBuffer == nullptr) {
            ERR_LOG << "Invalid record data";
            delete[] newRecordBuffer;
            return;
        }

        // Copy old data of recorded buffer to new allocated buffer and delete the old one
        std::copy(worker->recordedBuffer, worker->recordedBuffer + worker->recordedSampleCount, newRecordBuffer);
        delete[] worker->recordedBuffer;

        // keep track to new allocated buffer N buffer size
        worker->recordedBuffer = newRecordBuffer;
        worker->recordBufferSize = newBufferSize;
        WRN_LOG << "Allocate new buffer";
    }

    // Append stream into recorded buffer and update recorded samples count
    SDL_memcpy(worker->recordedBuffer + worker->recordedSampleCount, stream, len);
    worker->recordedSampleCount += newSamplesLength;
}

SonantWorker::SonantWorker(QObject *parent)
    : QObject { parent }
    , recordedBuffer { nullptr }
    , recordedSampleCount { 0 }
    , recordBufferSize { 0 }
    , m_initialized { false }
    , m_recording { false }
    , m_processing { false }
    , m_audioDevice { 0 }
    , m_whisperCtx { nullptr }
    , m_whisperModel { QString() }
    , m_transcription { QStringList() }
{
}

SonantWorker::~SonantWorker()
{
    DBG_LOG << "Close device and exit SDL";
    if (m_initialized) {
        SDL_Quit();
        whisper_free(m_whisperCtx);
    }
}

void SonantWorker::initialize()
{
    INF_LOG << "Initializing SDL...";
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        ERR_LOG << "SDL_Init error:" << SDL_GetError();
        return;
    }

    INF_LOG << "Setup record specs";
    m_recordSpecs.freq = RECORD_SAMPLE_RATE;
    m_recordSpecs.format = RECORD_SAMPLE_FORMAT;
    m_recordSpecs.channels = RECORD_CHANNELS;
    m_recordSpecs.samples = BUFFER_SIZE;
    m_recordSpecs.callback = audio_callback;
    m_recordSpecs.userdata = this;

    if (m_whisperCtx == nullptr) {
        INF_LOG << "Initializing whisper from model:" << m_whisperModel;
        m_whisperCtx = whisper_init_from_file(DEFAULT_WHISPER_MODEL);
        if (!m_whisperCtx) {
            ERR_LOG << "Fail to init whisper context.";
            return;
        }
    }

    m_initialized = true;
}

void SonantWorker::setWhisperModel(QString modelPath)
{
    if (m_recording || m_processing) {
        ERR_LOG << "Busy. Cannot set whisper model right now.";
        return;
    }

    if (m_whisperCtx != nullptr && modelPath == m_whisperModel)
        return;

    whisper_context *oldContext = m_whisperCtx;
    m_whisperModel = modelPath;
    INF_LOG << "Initializing whisper from model:" << m_whisperModel;
    m_whisperCtx = whisper_init_from_file(m_whisperModel.toStdString().c_str());
    if (!m_whisperCtx) {
        WRN_LOG << "Fail to init whisper context. Fallback to old context";
        if (oldContext != nullptr) {
            m_whisperCtx = oldContext;
        } else {
            ERR_LOG << "Old context is nullptr";
        }
    }
    else {
        // If init OK -> delete the old one
        if (oldContext != nullptr) {
            whisper_free(oldContext);
        }
    }
}

QStringList SonantWorker::getLatestTranscription() const
{
    return m_transcription;
}

int SonantWorker::startRecord()
{
    if (!m_initialized) {
        ERR_LOG << "Worker was not initialized.";
        return 1;
    }
    if (m_recording || m_processing) {
        ERR_LOG << "Busy.";
        return 2;
    }

    INF_LOG << "Prepare memory";
    recordedBuffer = new Sint16[RECORD_BUFFER_SIZE]{0};
    if (recordedBuffer == nullptr) {
        ERR_LOG << "Fail to allocate memory.";
        return 3;
    }
    recordBufferSize = RECORD_BUFFER_SIZE;

    INF_LOG << "Open audio device";

    m_audioDevice = SDL_OpenAudioDevice(nullptr, 1, &m_recordSpecs, nullptr, 0);
    if (m_audioDevice == 0) {
        ERR_LOG << "SDL_OpenAudioDevice error:" << SDL_GetError();
        return 4;
    }

    INF_LOG << "Start recording";
    m_recording = true;
    SDL_PauseAudioDevice(m_audioDevice, 0);

    SDL_Delay(RECORD_DURATION * 1000);

    INF_LOG << "Stop recording";
    SDL_PauseAudioDevice(m_audioDevice, 1);
    m_recording = false;

    SDL_CloseAudioDevice(m_audioDevice);
    emit recordCompleted();

    DBG_LOG << "recordedSampleCount:" << recordedSampleCount;
    DBG_LOG << "recordBufferSize" << recordBufferSize;

    writeRecordToFile(QLatin1String("record.wav"));
    processSpeech();
    return 0;
}

int SonantWorker::writeRecordToFile(QString filePath)
{
    if (!m_initialized) {
        ERR_LOG << "Worker was not initialized.";
        return 1;
    }
    if (m_recording || m_processing) {
        ERR_LOG << "Busy.";
        return 2;
    }
    if (recordedBuffer == nullptr) {
        ERR_LOG << "There is no record";
        return 2;
    }

    INF_LOG << "Write record to:" << filePath;
    SF_INFO sfinfo;
    sfinfo.samplerate = RECORD_SAMPLE_RATE;
    sfinfo.channels = RECORD_CHANNELS;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sndfile = sf_open(filePath.toStdString().c_str(), SFM_WRITE, &sfinfo);
    if (!sndfile) {
        ERR_LOG << "Fail to open file to write, error:" << sf_strerror(NULL);
        return 3;
    }
    sf_write_short(sndfile, recordedBuffer, recordedSampleCount);
    sf_close(sndfile);
    INF_LOG << "Write record completed.";

#ifdef AUDIO_DEBUG
    // init audioData;
    float *audioData = new float[recordedSampleCount]();

    // Write every value to file
    const char* filename = "audio_data_from_buffer.txt";
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Failed to open file for writing.\n");
        delete[] audioData;
        return 1;
    }

    for (unsigned long i = 0; i < recordedSampleCount; i++) {
        // Convert the 16-bit signed integer sample to a float and normalize it to [-1.0, 1.0]
        audioData[i] = (float)recordedBuffer[i] / 32768.0f;
        fprintf(file, "%f\n", audioData[i]);
    }

    fclose(file);
    delete[] audioData;
#endif
    return 0;
}

int SonantWorker::processSpeech()
{
    INF_LOG << "Processing speech...";
    if (!m_initialized) {
        ERR_LOG << "Worker was not initialized.";
        return 1;
    }
    if (m_recording || m_processing) {
        ERR_LOG << "Busy.";
        return 2;
    }

    m_processing = true;
    // init audioData;
    float *audioData = new float[recordedSampleCount]();
    if (!audioData) {
        ERR_LOG << "Failed to allocate memory for audioData.";
        return 1;
    }

    for (unsigned long i = 0; i < recordedSampleCount; i++) {
        audioData[i] = recordedBuffer[i] / MAX_SINT16_TO_FLOAT;
    }

    // Define parameters for transcription
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = PROCESS_COUNT;
    params.print_special = false; // Print special tokens like <SOT>, <EOT>, etc.
    params.print_progress = false; // Print progress information

    // Perform audio transcription
    int result = whisper_full(m_whisperCtx, params, audioData, recordedSampleCount);
    if (result != 0) {
        ERR_LOG << "Fail to transcribe audio.";
        return 2;
    }

    int segmentsCount = whisper_full_n_segments(m_whisperCtx);
    for (int i = 0; i < segmentsCount; i++) {
        m_transcription << QString(whisper_full_get_segment_text(m_whisperCtx, i));
    }

    delete[] audioData;
    m_processing = false;
    emit transcriptionReady();
    INF_LOG << "Done";
    return 0;
}
