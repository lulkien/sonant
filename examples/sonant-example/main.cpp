#include <QCoreApplication>
#include "sonantmanager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    SonantManager sonant;
//    sonant.initialize();
    sonant.startRecording();

    return a.exec();
}
