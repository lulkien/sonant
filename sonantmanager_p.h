#ifndef SONANTMANAGER_P_H
#define SONANTMANAGER_P_H

#include "sonantmanager.h"
#include "sonantworker.h"
#include <QThread>
#include <QString>

class Q_DECL_HIDDEN SonantManagerPrivate
{
    Q_DECLARE_PUBLIC(SonantManager)
public:
    SonantManagerPrivate(SonantManager *q_ptr);
    ~SonantManagerPrivate();

    void initialize();
    void fatality(const QString &errorString);

    void record();
    QString transcription();

    bool initialized;
    bool processing;
    SonantWorker voiceRecognizeWorker;
    QThread *voiceRecognizeThread;

    SonantManager *q_ptr;
};


#endif // SONANTMANAGER_P_H
