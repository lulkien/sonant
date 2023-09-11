#ifndef SONANTMANAGER_P_H
#define SONANTMANAGER_P_H

#include "sonantmanager.h"
#include "sonantworker.h"
#include <QtCore/qobject.h>
#include <QtCore/qthread.h>
#include <QtCore/qstringlist.h>

class SonantManagerPrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(SonantManager)
public:
    SonantManagerPrivate(SonantManager *_ptr);
    ~SonantManagerPrivate();

    // private implement
    void setModel(const QString &modelPath);
    void initialize();
    void record();
    QStringList transcription() const;

    // private class
    SonantManager   *q_ptr;

private:
    bool            m_initialized;
    SonantWorker    *m_sonantWorker;
    QThread         m_sonantWorkThread;
    QStringList     m_transcription;

private slots:
    void onRecordCompleted();
    void onTranscriptionReady();

signals:
    void requestChangeModel(const QString &modelPath);
    void requestWorkerInitialize();
    void requestRecord();
};


#endif // SONANTMANAGER_P_H
