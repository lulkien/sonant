#include <SDL2/SDL_stdinc.h>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

// SDL2 for recording
#include <SDL2/SDL.h>

// whisper for voice processing
#include <whisper.h>

class SonantImpl {
public:
    SonantImpl();
    virtual ~SonantImpl();
    void initialize();

    bool setModel(std::string path);
    std::string getModel() const;

    void setRecordThreshold(Uint16 threshold);
    bool startRecorder();
    void stopRecorder();
    void terminate();
    void setCallbackTranscriptionReady(std::function<void(std::string)> callback);

private:
    void recordAudio();
    bool reloadModel();

    friend void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);

private:
    // ---------------------------- Record thread ----------------------------
    // State of SDL recorder, listening or not
    std::atomic<bool> m_listening { false };

    // State of SDL recorder, recording or not
    std::atomic<bool> m_recording { false };

    // Interupt condition
    std::atomic<bool> m_terminate { false };

    // Thread which is recorder run on
    std::thread m_recorderThread;

    // ---------------------------- SDL2 ----------------------------
    // SDL initialize state: true/false
    bool m_SDLInitOk = false;

    /*
     * SDL2 device ID open state
     * 0: Failed to open device.
     * !0: Device open successfully (in normal case, it should be 2)
     * It will never be 1 for some compatible reasons.
    */
    SDL_AudioDeviceID m_deviceId = 0;

    // Actual spec of the record device
    SDL_AudioSpec m_obtainedSpec;

    // Volume threshold of recorder
    Uint16 m_recordThreshold = 4000;

    // Record buffer, will be passed to whisper to process
    std::vector<Uint8> m_recordBuffer;

    // Mutex buffer
    std::mutex m_bufferMutex;

    // Last moment still get input
    std::chrono::steady_clock::time_point m_lastInputTime;

    // Delay before stop record and pass buffer to whisper
    std::chrono::seconds m_stopRecordDelay = std::chrono::seconds(1);

    // ---------------------------- Whisper ----------------------------
    // State of whisper, processing or not
    std::atomic<bool> m_whisperProcessing { false };

    // Whisper context, load model to get this context
    whisper_context *m_context = nullptr;

    // Whisper model path, no model, no context
    std::string m_modelPath { "" };

    // Callback when whisper completed process record buffer
    std::function<void(std::string)> m_callbackTranscriptionReady;
};
