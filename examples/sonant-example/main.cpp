#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sonant.h>
#include <string>
#include <thread>
#include <unistd.h> // For sleep
#include <signal.h>

Sonant sonant;
std::atomic<bool> is_stop { false };

void handleSignal(int signal)
{
    if (signal == SIGINT) {
        std::cout << "\nTerminated\n";
        sonant.stopRecorder();
        sonant.terminate();
        is_stop = true;
    }
}

void printTranscript(std::string transcript) {
    std::cout << transcript << std::endl;
}

int main() {
    signal(SIGINT, handleSignal);

    // Use script download-ggml-model.sh in Whisper.cpp project to download Whisper model
    if (!sonant.initialize("ggml-base.en.bin")) {
        return 0;
    }

    sonant.startRecorder();
    sonant.setTranscriptionCallback(printTranscript);

    while (!is_stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Stop" << std::endl;
    /*std::this_thread::sleep_for(std::chrono::milliseconds(25000));*/
    /*sonant.stopRecorder();*/


    return 0;
}

