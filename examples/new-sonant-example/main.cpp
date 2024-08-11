#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sonant.h>
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

int main() {
    signal(SIGINT, handleSignal);

    sonant.setModel("a");

    sonant.initialize();
    sonant.startRecorder();

    while (!is_stop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Stop" << std::endl;
    /*sonant.startRecorder();*/
    /*std::this_thread::sleep_for(std::chrono::milliseconds(25000));*/
    /*sonant.stopRecorder();*/


    return 0;
}

