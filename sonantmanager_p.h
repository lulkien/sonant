#ifndef SONANTMANAGER_P_H
#define SONANTMANAGER_P_H

#include "sonantmanager.h"
#include "sonantworker.h"
#include <QObject>
#include <QThread>
#include <QString>

class Q_DECL_HIDDEN SonantManagerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(SonantManager)
public:
    SonantManagerPrivate(SonantManager *q_ptr);
    ~SonantManagerPrivate();

    void initialize();

    void record();
    QStringList getTranscription() const;

    bool initialized;
    SonantWorker voiceRecognizeWorker;
    QThread *voiceRecognizeThread;
    QStringList transcription;

    // private class
    SonantManager *q_ptr;

private slots:
    void getTranscriptionFromWorker();

signals:
    void transcriptionReady();
};


#endif // SONANTMANAGER_P_H
