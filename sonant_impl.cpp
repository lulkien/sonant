#include "sonant_impl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_stdinc.h>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>

// Callback for SDL recording
void SDLCALL audioCallback(void* userdata, Uint8* stream, int len) {
    if (len <= 0) {
        return;
    }

    SonantImpl* _ptr = reinterpret_cast<SonantImpl*>(userdata);
    if (!_ptr) {
        std::cout << "Invalid userdata" << std::endl;
        return;
    }

    // If any sample above threshold -> record
    Sint16* samples = reinterpret_cast<Sint16*>(stream);
    Uint16 sampleCount = len / (sizeof(Sint16) / sizeof(Uint8));
    bool record_ok = false;

    for (int i = 0; i < sampleCount; i++) {
        if (abs(samples[i]) > _ptr->m_recordThreshold) {
            record_ok = true;
            break;
        }
    }

    // Write to record
    if (record_ok) {
        std::lock_guard<std::mutex> lock(_ptr->m_bufferMutex);
        _ptr->m_recording.store(true);
        _ptr->m_audioBuffer.insert(_ptr->m_audioBuffer.end(), stream, stream + len);
    }
}

SonantImpl::SonantImpl() {
    SDL_Init(SDL_INIT_AUDIO);
}

SonantImpl::~SonantImpl() {
    stopRecorder();

    SDL_CloseAudioDevice(m_deviceId);
    SDL_Quit();

    terminate();
}

void SonantImpl::initialize() {
    // Init SDL2 subsystem
    SDL_AudioSpec desiredSpec;

    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = 44100; // 44.1kHz
    desiredSpec.format = AUDIO_S16LSB; // 16-bit signed
    desiredSpec.channels = 1; // Mono audio
    desiredSpec.samples = 4096; // Buffer size
    desiredSpec.callback = audioCallback;
    desiredSpec.userdata = this;

    m_deviceId = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &desiredSpec, &m_obtainedSpec, 0);

    if (m_deviceId == 0) {
        m_SDLInitOk = false;
        std::cerr << "Failed to open audio device: " << SDL_GetError() << "\n";
        SDL_Quit();
    } else {
        m_SDLInitOk = true;
        // Start record thread
        m_recorderThread = std::thread(&SonantImpl::recordAudio, this);
    }

}

bool SonantImpl::setModel(std::string path) {
    if (path != m_modelPath) {
        m_modelPath = path;
    }

    return true;
}

std::string SonantImpl::getModel() const {
    return m_modelPath;
}

bool SonantImpl::startRecorder() {
    if (m_modelPath.empty()) {
        std::cerr << __PRETTY_FUNCTION__ << "Model path is not set." << std::endl;
        return false;
    }

    if (!m_SDLInitOk) {
        std::cerr << __PRETTY_FUNCTION__ << "Cannot init SDL2 subsystem" << std::endl;
        return false;
    }

    // Unpause mic
    SDL_PauseAudioDevice(m_deviceId, SDL_FALSE);

    // Record audio
    m_listening.store(true);
    return true;
}

bool SonantImpl::stopRecorder() {
    if (!m_listening.load()) {
        std::cout << "Recording was not running.\n";
        return false;
    }

    std::cout << "Stop\n";
    m_listening.store(false);

    return true;
}

void SonantImpl::setCallbackTranscriptionReady(std::function<void(std::string)> callback) {
    m_callbackTranscriptionReady = callback;
}

void SonantImpl::terminate() {
    if (!m_terminate.load()) {
        m_terminate.store(true);
    }

    if (m_recorderThread.joinable()) {
        m_recorderThread.join();
    }
}

void SonantImpl::recordAudio() {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    auto lastInputTime = steady_clock::now();

    while (true) {
        if (m_terminate.load()) {
            break;
        }

        if (m_SDLInitOk && m_listening.load()) {
            auto now = steady_clock::now();
            auto durationSinceLastInput = duration_cast<seconds>(now - lastInputTime);

            if (m_recording.load()) {
                std::cout << "\r" << std::string(100, ' ') << "\r";
                std::cout << "Recording...";
                std::cout.flush();
            } else {
                std::cout << "\r" << std::string(100, ' ') << "\r";
                std::cout << "Idle";
                std::cout.flush();
            }

        }

        // sleep
        sleep_for(milliseconds(100));
    }

    std::cout << "Terminate record thread" << std::endl;
}


bool SonantImpl::reloadModel() {
    return true;
}
