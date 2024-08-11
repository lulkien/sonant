#include <cstdlib>
#include <iostream>
#include <sonant.h>
#include <unistd.h> // For sleep
#include <signal.h>

Sonant sonant;

void handleSignal(int signal)
{
    if (signal == SIGINT) {
        printf("Terminated.\n");
        sonant.stopRecorder();
        sonant.terminate();
    }
}

int main() {
    signal(SIGINT, handleSignal);

    sonant.setModel("a");

    sonant.initialize();
    sonant.startRecorder();

    std::cout << "Stop" << std::endl;
    /*sonant.startRecorder();*/
    /*std::this_thread::sleep_for(std::chrono::milliseconds(25000));*/
    /*sonant.stopRecorder();*/


    return 0;
}

