#include <QCoreApplication>
#include <signal.h>
#include "sonantmanager.h"

QCoreApplication *app;

void handleSignal(int signal)
{
    if (signal == SIGINT) {
        printf("Terminated.\n");
//        abort();
        app->quit();
    }
}

int main(int argc, char *argv[])
{
    // Register the SIGINT signal handler
    signal(SIGINT, handleSignal);

    QCoreApplication a(argc, argv);
    app = &a;

    SonantManager sonant;
    sonant.initialize();
    sonant.startRecording();

    return a.exec();
}
