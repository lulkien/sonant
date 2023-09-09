#include "sonantmanager.h"
#include "sonantmanager_p.h"
#include "common.h"
#include <QDebug>

SonantManager::SonantManager()
    : d_ptr { new SonantManagerPrivate(this) }
{
    DBG_LOG;
    connect(d_ptr.get(), &SonantManagerPrivate::transcriptionReady, this, &SonantManager::transcriptionReady);
}

SonantManager::~SonantManager()
{
    DBG_LOG;
}

void SonantManager::initialize()
{
    DBG_LOG;
    Q_D(SonantManager);
    d->initialize();
}

void SonantManager::startRecording()
{
    DBG_LOG << QThread::currentThread()->currentThreadId();
    Q_D(SonantManager);
    d->record();
}

QStringList SonantManager::getTranscription()
{
    DBG_LOG;
    Q_D(SonantManager);
    return d->getTranscription();
}

SonantManagerPrivate::SonantManagerPrivate(SonantManager *q_ptr)
{
    DBG_LOG;
    this->q_ptr = q_ptr;
    this->initialized = false;
    this->sonantWorkThread = nullptr;
    this->sonantWorker = nullptr;
}

SonantManagerPrivate::~SonantManagerPrivate()
{
    DBG_LOG;
    this->sonantWorkThread->quit();
    this->sonantWorkThread->wait();

    if (this->sonantWorker)
        delete this->sonantWorker;

    if (this->sonantWorkThread)
        delete this->sonantWorkThread;
}

void SonantManagerPrivate::initialize()
{
    DBG_LOG;
    this->sonantWorkThread = new QThread();
    this->sonantWorkThread->moveToThread(sonantWorkThread);

    this->sonantWorker = new SonantWorker();
    this->sonantWorker->initialize();
    INF_LOG << "Worker thread ID:" << sonantWorkThread->currentThreadId();

    // connect signals/slots
    connect(sonantWorker, &SonantWorker::transcriptionReady,
            this, &SonantManagerPrivate::getTranscriptionFromWorker, Qt::QueuedConnection);
    connect(this, &SonantManagerPrivate::requestRecord,
            sonantWorker, &::SonantWorker::onRequestRecord, Qt::QueuedConnection);

    // Start thread
    this->sonantWorkThread->start();

    // done
    this->initialized = true;
}

void SonantManagerPrivate::record()
{
    DBG_LOG << QThread::currentThread()->currentThreadId();
    if (!this->initialized) {
        ERR_LOG << "Manager is not initialized";
        return;
    }
    emit requestRecord();
}

QStringList SonantManagerPrivate::getTranscription() const
{
    DBG_LOG;
    if (!this->initialized) {
        ERR_LOG << "Manager is not initialized";
        return QStringList();
    }

    return this->transcription;
}

void SonantManagerPrivate::testFunction()
{
    for (int i = 0; i < this->transcription.count(); i++) {
        INF_LOG << "Segment" << i << ":" << this->transcription.at(i);
    }
}

void SonantManagerPrivate::getTranscriptionFromWorker()
{
    this->transcription = sonantWorker->getLatestTranscription();
#ifdef DUMMY
    testFunction();
#endif
    emit transcriptionReady();
}

