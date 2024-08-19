#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

// ALSA for recording
#include <alsa/asoundlib.h>

// whisper for voice processing
#include <whisper.h>

class SonantImpl {
public:
    SonantImpl();
    virtual ~SonantImpl();

    bool initialize(const std::string& modelPath);

    bool setModel(const std::string& modelPath);
    std::string getModel() const;

    void setRecordThreshold(float_t threshold);
    float_t getRecordThreshold() const;

    bool startRecorder();
    void stopRecorder();
    void terminate();

    void setCallbackTranscriptionReady(std::function<void(std::string)> callback);

private:
    void doRecordAudio();
    void alsaCaptureHandle(std::vector<float_t> &buffer);

    void processRecordBuffer();
    bool reloadModel(const std::string& newModelPath);

private:
    // ---------------------------- Record thread ----------------------------
    std::atomic<bool>     m_listening { false };
    std::atomic<bool>     m_recording { false };
    std::atomic<bool>     m_terminate { false };

    float_t               m_recordThreshold = 0.25; // Default: 25%
    std::vector<float_t>  m_recordBuffer;
    std::mutex            m_bufferMutex;
    std::thread           m_recorderThread;

    // ---------------------------- ALSA ----------------------------
    bool                                    m_alsaInitOk = false;
    snd_pcm_t*                              m_alsaCapture;

    std::chrono::steady_clock::time_point   m_lastInputTime;
    std::chrono::seconds                    m_stopRecordDelay = std::chrono::seconds(2);

    // ---------------------------- Whisper ----------------------------
    bool                              m_whisperInitOk = false;
    std::atomic<bool>                 m_whisperProcessing { false };
    whisper_context*                  m_whisperCtx = nullptr;
    std::string                       m_whisperModelPath = "";

    std::function<void(std::string)>  m_callbackTranscriptionReady;
};
