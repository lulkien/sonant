#ifndef SONANT_IMPL_H
#define SONANT_IMPL_H

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <alsa/asoundlib.h>
#include <whisper.h>

struct SonantParams;
class SonantImpl {
public:
    SonantImpl();
    virtual ~SonantImpl();

    bool initialize(const std::string &modelPath, const SonantParams &params);
    bool requestChangeModel(const std::string &newModelPath);

    bool startRecorder();
    void stopRecorder();
    void terminate();

    void
    setCallbackTranscriptionReady(std::function<void(std::string)> callback);

private:
    void doRecordAudio();
    void alsaCaptureHandle(std::vector<float> &buffer);

    void processRecordBuffer();
    bool reloadModel(const std::string &newModelPath);

private:
    // ---------------------------- Record thread ----------------------------
    std::atomic<bool> m_recording{false};
    std::atomic<bool> m_terminate{false};

    float m_recordThreshold = 0.25; // Default: 25%
    std::vector<float> m_recordBuffer;
    std::mutex m_bufferMutex;
    std::thread m_recorderThread;

    // ---------------------------- ALSA ----------------------------
    bool m_alsaInitOk = false;
    snd_pcm_t *m_alsaCapture;

    std::chrono::steady_clock::time_point m_lastInputTime;
    std::chrono::milliseconds m_stopRecordDelay =
        std::chrono::milliseconds(1500);

    // ---------------------------- Whisper ----------------------------
    bool m_whisperInitOk = false;
    whisper_context *m_whisperCtx = nullptr;
    whisper_full_params m_whisperParams;
    std::string m_whisperModelPath;
    std::mutex m_whisperMutex;

    std::function<void(std::string)> m_callbackTranscriptionReady;
};

#endif // !SONANT_IMPL_H
#define SONANT_IMPL_H
