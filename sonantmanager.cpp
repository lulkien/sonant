#include "sonantmanager.h"
#include "sonantmanager_p.h"
#include <QDebug>

SonantManager::SonantManager()
    : d_ptr { new SonantManagerPrivate(this) }
{
    qDebug();
    connect(d_ptr.get(), &SonantManagerPrivate::transcriptionReady, this, &SonantManager::transcriptionReady);
}

SonantManager::~SonantManager()
{
    qDebug();
}

void SonantManager::initialize()
{
    qDebug();
    Q_D(SonantManager);
    d->initialize();
}

void SonantManager::startRecording()
{
    qDebug() << QThread::currentThread()->currentThreadId();
    Q_D(SonantManager);
    d->record();
}

QStringList SonantManager::getTranscription()
{
    qDebug();
    Q_D(SonantManager);
    return d->getTranscription();
}

SonantManagerPrivate::SonantManagerPrivate(SonantManager *q_ptr)
{
    this->q_ptr = q_ptr;
    this->initialized = false;
    this->sonantWorkThread = nullptr;
    this->sonantWorker = nullptr;
}

SonantManagerPrivate::~SonantManagerPrivate()
{
    this->sonantWorkThread->quit();
    this->sonantWorkThread->wait();

    if (this->sonantWorker)
        delete this->sonantWorker;

    if (this->sonantWorkThread)
        delete this->sonantWorkThread;
}

void SonantManagerPrivate::initialize()
{
    this->sonantWorkThread = new QThread();
    this->sonantWorkThread->moveToThread(sonantWorkThread);

    this->sonantWorker = new SonantWorker();
    this->sonantWorker->initialize();
    qInfo() << "Worker thread ID:" << sonantWorkThread->currentThreadId();

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
    qDebug() << QThread::currentThread()->currentThreadId();
    if (!this->initialized) {
        qCritical() << "Manager is not initialized";
        return;
    }
    emit requestRecord();
}

QStringList SonantManagerPrivate::getTranscription() const
{
    if (!this->initialized) {
        qCritical() << "Manager is not initialized";
        return QStringList();
    }
    return this->transcription;
}

void SonantManagerPrivate::testFunction()
{
    for (int i = 0; i < this->transcription.count(); i++) {
        qInfo() << "Segment" << i << ":" << this->transcription.at(i);
    }
}

void SonantManagerPrivate::getTranscriptionFromWorker()
{
    this->transcription = sonantWorker->getLatestTranscription();
    emit transcriptionReady();
}

