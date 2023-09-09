#include <QCoreApplication>
#include <signal.h>
#include "sonantmanager.h"

void handleSignal(int signal)
{
    if (signal == SIGINT) {
        printf("Terminated.\n");
        abort();
    }
}

int main(int argc, char *argv[])
{
    // Register the SIGINT signal handler
    signal(SIGINT, handleSignal);

    QCoreApplication a(argc, argv);

    SonantManager sonant;
    sonant.initialize();
    sonant.startRecording();

    return a.exec();
}
