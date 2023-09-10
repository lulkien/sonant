#include "sonantworker.h"
#include <QDebug>
#include <QThread>

#define RECORD_SAMPLE_RATE      16000
#define RECORD_CHANNELS         1
#define RECORD_SAMPLE_FORMAT    AUDIO_S16LSB
#define RECORD_BUFFER_SIZE      16384

#define DEFAULT_WHISPER_MODEL   "models/ggml-tiny.en.bin"
#define MAX_SINT16_TO_FLOAT     32768.0f
#define PROCESS_COUNT           6

#define RECORD_DURATION         2

void audio_callback(void* userdata, Uint8* stream, int len)
{
    Q_UNUSED(stream)
    Q_UNUSED(len)
    if (userdata == nullptr) {
        qCritical() << "userdata is nullptr, no where to write";
        return;
    }

    SonantWorker *worker = static_cast<SonantWorker *>(userdata);

    // Ensure that the audio buffer has enough space
    Uint32 newSamplesLength = (len / sizeof(Sint16));
    Uint32 newBufferSize = worker->recordedSampleCount + newSamplesLength + 1;

    if (newBufferSize > worker->recordBufferSize) {
        Sint16 *newRecordBuffer = new Sint16[newBufferSize]{0};
        if (newRecordBuffer == nullptr) {
            qCritical() << "Fail to allocate new buffer";
            return;
        }

        if (worker->recordedBuffer == nullptr) {
            qCritical() << "Invalid record data";
            delete[] newRecordBuffer;
            return;
        }

        // Copy old data of recorded buffer to new allocated buffer and delete the old one
        std::copy(worker->recordedBuffer, worker->recordedBuffer + worker->recordedSampleCount, newRecordBuffer);
        delete[] worker->recordedBuffer;

        // keep track to new allocated buffer N buffer size
        worker->recordedBuffer = newRecordBuffer;
        worker->recordBufferSize = newBufferSize;
        qWarning() << "Allocate new buffer";
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
    qDebug() << "Close device and exit SDL";
    if (m_initialized) {
        SDL_Quit();
        whisper_free(m_whisperCtx);
    }
}

void SonantWorker::initialize()
{
    qInfo() << "Initializing SDL...";
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qCritical() << "SDL_Init error:" << SDL_GetError();
        return;
    }

    qInfo() << "Setup record specs";
    m_recordSpecs.freq = RECORD_SAMPLE_RATE;
    m_recordSpecs.format = RECORD_SAMPLE_FORMAT;
    m_recordSpecs.channels = RECORD_CHANNELS;
    m_recordSpecs.samples = RECORD_BUFFER_SIZE;
    m_recordSpecs.callback = audio_callback;
    m_recordSpecs.userdata = this;

    if (m_whisperCtx == nullptr) {
        qInfo() << "Initializing whisper from model";
        m_whisperCtx = whisper_init_from_file(DEFAULT_WHISPER_MODEL);
        if (!m_whisperCtx) {
            qCritical() << "Fail to init whisper context.";
            return;
        }
    }

    m_initialized = true;
}

void SonantWorker::setWhisperModel(QString modelPath)
{
    if (m_recording || m_processing) {
        qCritical() << "Busy. Cannot set whisper model right now.";
        return;
    }

    if (m_whisperCtx != nullptr && modelPath == m_whisperModel)
        return;

    whisper_context *oldContext = m_whisperCtx;
    m_whisperModel = modelPath;
    qInfo() << "Initializing whisper from model:" << m_whisperModel;
    m_whisperCtx = whisper_init_from_file(m_whisperModel.toStdString().c_str());
    if (!m_whisperCtx) {
        qWarning() << "Fail to init whisper context. Fallback to old context";
        if (oldContext != nullptr) {
            m_whisperCtx = oldContext;
        } else {
            qCritical() << "Old context is nullptr";
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

void SonantWorker::onRequestRecord()
{
    qDebug() << QThread::currentThread()->currentThreadId();
    if (!m_initialized) {
        qCritical() << "Worker was not initialized.";
        return;
    }
    if (m_recording || m_processing) {
        qCritical() << "Busy.";
        return;
    }
    startRecord();
}

int SonantWorker::startRecord()
{
    qDebug() << QThread::currentThread()->currentThreadId();
    if (!m_initialized) {
        qCritical() << "Worker was not initialized.";
        return 1;
    }
    if (m_recording || m_processing) {
        qCritical() << "Busy.";
        return 2;
    }

    qInfo() << "Prepare memory";
    recordedBuffer = new Sint16[RECORD_BUFFER_SIZE]{0};
    if (recordedBuffer == nullptr) {
        qCritical() << "Fail to allocate memory.";
        return 3;
    }
    recordBufferSize = RECORD_BUFFER_SIZE;

    qInfo() << "Open audio device";

    m_audioDevice = SDL_OpenAudioDevice(nullptr, 1, &m_recordSpecs, nullptr, 0);
    if (m_audioDevice == 0) {
        qCritical() << "SDL_OpenAudioDevice error:" << SDL_GetError();
        return 4;
    }

    qInfo() << "Start recording";
    m_recording = true;
    SDL_PauseAudioDevice(m_audioDevice, 0);

    SDL_Delay(RECORD_DURATION * 1000);

    qInfo() << "Stop recording";
    SDL_PauseAudioDevice(m_audioDevice, 1);
    m_recording = false;

    SDL_CloseAudioDevice(m_audioDevice);
    emit recordCompleted();

    qDebug() << "recordedSampleCount:" << recordedSampleCount;
    qDebug() << "recordBufferSize" << recordBufferSize;

    processSpeech();
    return 0;
}

int SonantWorker::processSpeech()
{
    qInfo() << "Processing speech...";
    if (!m_initialized) {
        qCritical() << "Worker was not initialized.";
        return 1;
    }
    if (m_recording || m_processing) {
        qCritical() << "Busy.";
        return 2;
    }

    m_processing = true;
    // init audioData;
    float *audioData = new float[recordedSampleCount]();
    if (!audioData) {
        qCritical() << "Failed to allocate memory for audioData.";
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
        qCritical() << "Fail to transcribe audio.";
        return 2;
    }

    int segmentsCount = whisper_full_n_segments(m_whisperCtx);
    for (int i = 0; i < segmentsCount; i++) {
        m_transcription << QString(whisper_full_get_segment_text(m_whisperCtx, i));
    }

    delete[] audioData;
    m_processing = false;
    emit transcriptionReady();
    qInfo() << "Done";
    return 0;
}
