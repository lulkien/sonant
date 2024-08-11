#include <SDL2/SDL_stdinc.h>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

// SDL2 for recording
#include <SDL2/SDL.h>

// whisper for voice processing
#include <vector>
#include <whisper.h>

class SonantImpl {
public:
    SonantImpl();
    virtual ~SonantImpl();
    void initialize();

    bool setModel(std::string path);
    std::string getModel() const;

    bool startRecorder();
    bool stopRecorder();

    void setCallbackTranscriptionReady(std::function<void(std::string)> callback);

    void terminate();

private:
    void recordAudio();

    bool reloadModel();

    friend void SDLCALL audioCallback(void* userdata, Uint8* stream, int len);

private:
    std::atomic<bool> m_listening { false };
    std::atomic<bool> m_recording { false };
    std::atomic<bool> m_terminate { false };
    std::thread m_recorderThread;

    std::string m_modelPath { "" };
    std::function<void(std::string)> m_callbackTranscriptionReady;

    // SDL2
    bool m_SDLInitOk = false;
    SDL_AudioDeviceID m_deviceId = 0;
    SDL_AudioSpec m_obtainedSpec;
    int m_recordThreshold = 4000;
    std::vector<Uint8> m_audioBuffer;
    std::mutex m_bufferMutex;

    // whisper
    std::atomic<bool> m_whisperProcessing { false };
    whisper_context *m_context = nullptr;
};
