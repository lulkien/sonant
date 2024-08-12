#include "sonant_impl.h"
#include "sonant_utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_stdinc.h>
#include <chrono>
#include <cstdlib>
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

    if (nullptr == userdata) {
        ERR_LOG << "Invalid userdata" << std::endl;
        return;
    }

    SonantImpl* _ptr = reinterpret_cast<SonantImpl*>(userdata);

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
    if (record_ok || _ptr->m_recording.load()) {
        if (record_ok) {
            _ptr->m_recording.store(true);
            _ptr->m_lastInputTime = std::chrono::steady_clock::now();
        }

        std::lock_guard<std::mutex> lock(_ptr->m_bufferMutex);
        _ptr->m_recordBuffer.insert(_ptr->m_recordBuffer.end(), stream, stream + len);
    }
}

SonantImpl::SonantImpl() {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;

    // Convert 00:00:00 1/1/1970 to steady_clock::time_point
    m_lastInputTime = steady_clock::now() + duration_cast<steady_clock::duration>(seconds(0));
}

SonantImpl::~SonantImpl() {
    stopRecorder();

    SDL_CloseAudioDevice(m_deviceId);
    SDL_Quit();

    terminate();
}

void SonantImpl::initialize() {
    // Init SDL2 subsystem
    SDL_Init(SDL_INIT_AUDIO);
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
        ERR_LOG << "Failed to open audio device: " << SDL_GetError() << std::endl;
        SDL_Quit();
    } else {
        m_SDLInitOk = true;
        // Start record thread
        m_recorderThread = std::thread(&SonantImpl::recordAudio, this);
    }

    // Init whisper
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

void SonantImpl::setRecordThreshold(Uint16 threshold) {
    m_recordThreshold = threshold;
}

bool SonantImpl::startRecorder() {
    if (m_modelPath.empty()) {
        ERR_LOG << "Model path was not set." << std::endl;
        return false;
    }

    if (!m_SDLInitOk) {
        ERR_LOG << "SDL2 subsystem was not initialized correctly." << std::endl;
        return false;
    }

    // Unpause mic
    SDL_PauseAudioDevice(m_deviceId, SDL_FALSE);

    // Record audio
    m_listening.store(true);
    return true;
}

void SonantImpl::stopRecorder() {
    // Pause mic
    SDL_PauseAudioDevice(m_deviceId, SDL_TRUE);
    m_listening.store(false);
    return;
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

void SonantImpl::recordAudio() {
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    auto lastInputTime = steady_clock::now();

    while (true) {
        if (!m_SDLInitOk) {
            UNREACHABLE();
            break;
        }

        if (m_terminate.load()) {
            break;
        }

        if (m_listening.load()) {
            auto now = steady_clock::now();
            auto lastInputDeltaTime = duration_cast<seconds>(now - m_lastInputTime);

            if (m_recording.load()) {
                if (lastInputDeltaTime < m_stopRecordDelay) {
                    // Keep recording
                    m_recording.store(true);
                }
                else {
                    // Stop record
                    m_recording.store(false);
                    MSG_LOG << "Stop recorder after 1 seconds." << std::endl;

                    {
                        std::lock_guard<std::mutex> lock(m_bufferMutex);
                        MSG_LOG << "Pass buffer to whisper. Buffer size = "
                                << m_recordBuffer.size()
                                << std::endl;
                        m_recordBuffer.clear();
                    }
                }
            }

            if (m_recording.load()) {
                std::cout << "Recording..." << std::endl;
            } else {
                std::cout << "Listening..." << std::endl;
            }
        }

        // sleep
        sleep_for(milliseconds(100));
    }

    if (m_terminate.load()) {
        MSG_LOG << "Terminate record thread" << std::endl;
    } else {
        UNREACHABLE();
    }
}


bool SonantImpl::reloadModel() {
    return true;
}
