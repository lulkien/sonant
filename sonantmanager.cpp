#include "sonantmanager.h"
#include "sonantmanager_p.h"
#include <QtCore/qdebug.h>

SonantManager::SonantManager()
    : d_ptr { new SonantManagerPrivate(this) }
{ }

SonantManager::~SonantManager()
{ }

void SonantManager::setModel(const QString &modelPath)
{
    Q_D(SonantManager);
    d->setModel(modelPath);
}

void SonantManager::initialize()
{
    Q_D(SonantManager);
    d->initialize();
}

void SonantManager::record()
{
    Q_D(SonantManager);
    d->record();
}

QStringList SonantManager::transcription()
{
    Q_D(SonantManager);
    return d->transcription();
}

SonantManagerPrivate::SonantManagerPrivate(SonantManager *_ptr)
    : q_ptr { _ptr }
{
    m_initialized = false;
    m_sonantWorker = new SonantWorker();
    m_sonantWorker->moveToThread(&m_sonantWorkThread);

    // Start thread
    m_sonantWorkThread.start();
}

SonantManagerPrivate::~SonantManagerPrivate()
{
    m_sonantWorkThread.quit();
    m_sonantWorkThread.wait();

    if (m_sonantWorker)
        delete m_sonantWorker;
}

void SonantManagerPrivate::setModel(const QString &modelPath)
{
    emit requestChangeModel(modelPath);
}

void SonantManagerPrivate::initialize()
{
    // Worker -> Manager
    connect(m_sonantWorker, &SonantWorker::recordCompleted,
            this, &SonantManagerPrivate::onRecordCompleted, Qt::QueuedConnection);
    connect(m_sonantWorker, &SonantWorker::transcriptionReady,
            this, &SonantManagerPrivate::onTranscriptionReady, Qt::QueuedConnection);

    // Manager -> Worker
    connect(this, &SonantManagerPrivate::requestChangeModel,
            m_sonantWorker, &SonantWorker::onRequestChangeModel, Qt::QueuedConnection);
    connect(this, &SonantManagerPrivate::requestWorkerInitialize,
            m_sonantWorker, &SonantWorker::onRequestInitialize, Qt::QueuedConnection);
    connect(this, &SonantManagerPrivate::requestRecord,
            m_sonantWorker, &SonantWorker::onRequestRecord, Qt::QueuedConnection);

//    m_sonantWorker->initialize();
    emit requestWorkerInitialize();
    // done
    m_initialized = true;
}

void SonantManagerPrivate::record()
{
    if (!this->m_initialized) {
        qCritical() << "Manager is not initialized";
        return;
    }
    emit requestRecord();
}

QStringList SonantManagerPrivate::transcription() const
{
    if (!this->m_initialized) {
        qCritical() << "Manager is not initialized";
        return QStringList();
    }
    return m_transcription;
}

void SonantManagerPrivate::onRecordCompleted()
{
    // If the record can be completed, it must be initialized
    emit q_ptr->recordCompleted();
}

void SonantManagerPrivate::onTranscriptionReady()
{
    // If the transcription can be ready, it must be initialized
    m_transcription = m_sonantWorker->getLatestTranscription();
    emit q_ptr->transcriptionReady();
}

