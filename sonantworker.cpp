#include "sonantworker.h"
#include "common.h"
#include <QDebug>
#include <sndfile.h>

#define RECORD_SAMPLE_RATE      16000
#define RECORD_CHANNELS         1
#define RECORD_SAMPLE_FORMAT    AUDIO_S16LSB
#define RECORD_BUFFER_SIZE      81920

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
        Sint16 *newRecordBuffer = new Sint16[newBufferSize]();
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
{

}

SonantWorker::~SonantWorker()
{
    DBG_LOG << "Close device and exit SDL";
    if (m_initialized) {
        SDL_CloseAudioDevice(m_audioDevice);
        SDL_Quit();
    }
}

void SonantWorker::initialize()
{
    INF_LOG << "Initializing SDL...";
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        ERR_LOG << "SDL_Init error:" << SDL_GetError();
        return;
    }

    SDL_AudioSpec specs;
    specs.freq = RECORD_SAMPLE_RATE;
    specs.format = RECORD_SAMPLE_FORMAT;
    specs.channels = RECORD_CHANNELS;
    specs.samples = BUFFER_SIZE;
    specs.callback = audio_callback;
    specs.userdata = this;

    INF_LOG << "Open audio device";
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 1, &specs, nullptr, 0);
    if (m_audioDevice == 0) {
        ERR_LOG << "SDL_OpenAudioDevice error:" << SDL_GetError();
        return;
    }

    m_initialized = true;
}

int SonantWorker::startRecord()
{
    if (!m_initialized) {
        ERR_LOG << "Worker was not initialized.";
        return 1;
    }

    INF_LOG << "Prepare memory";
    recordedBuffer = new Sint16[RECORD_BUFFER_SIZE]();
    if (recordedBuffer == nullptr) {
        ERR_LOG << "Fail to allocate memory.";
        return 2;
    }
    recordBufferSize = RECORD_BUFFER_SIZE;

    INF_LOG << "Start recording";
    m_recording = true;
    SDL_PauseAudioDevice(m_audioDevice, 0);

    SDL_Delay(RECORD_DURATION * 1000);    // record 1 second

    INF_LOG << "Stop recording";
    SDL_PauseAudioDevice(m_audioDevice, 1);
    m_recording = false;

    writeRecordToFile(QLatin1String("record.wav"));

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

    DBG_LOG << "recordedSampleCount:" << recordedSampleCount;
    DBG_LOG << "recordBufferSize" << recordBufferSize;

    return 0;
}

int SonantWorker::writeRecordToFile(QString filePath)
{
    INF_LOG << "Write record to:" << filePath;
    SF_INFO sfinfo;
    sfinfo.samplerate = RECORD_SAMPLE_RATE;
    sfinfo.channels = RECORD_CHANNELS;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sndfile = sf_open(filePath.toStdString().c_str(), SFM_WRITE, &sfinfo);
    if (!sndfile) {
        ERR_LOG << "Fail to open file to write, erro:" << sf_strerror(NULL);
        return -1;
    }
    sf_write_short(sndfile, recordedBuffer, recordedSampleCount);
    sf_close(sndfile);

    INF_LOG << "Write record completed.";
    return 0;
}
