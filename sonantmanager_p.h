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

    // private class
    SonantManager *q_ptr;

private:
    bool initialized;
    SonantWorker *sonantWorker;
    QThread *sonantWorkThread;
    QStringList transcription;

private slots:
    void testFunction();
    void getTranscriptionFromWorker();

signals:
    void requestRecord();
    void transcriptionReady();
};


#endif // SONANTMANAGER_P_H
