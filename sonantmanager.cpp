#include "sonantmanager.h"
#include "sonantmanager_p.h"
#include "common.h"
#include <QDebug>

SonantManager::SonantManager()
    : d_ptr { new SonantManagerPrivate(this) }
{
    DBG_LOG;
}

SonantManager::~SonantManager()
{

}

void SonantManager::initialize()
{
    DBG_LOG;
    Q_D(SonantManager);
    d->initialize();
}

void SonantManager::startRecording()
{
    DBG_LOG;
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
    this->voiceRecognizeThread = nullptr;
}

SonantManagerPrivate::~SonantManagerPrivate()
{
    DBG_LOG;
}

void SonantManagerPrivate::initialize()
{
    DBG_LOG;
//    this->voiceRecognizeThread = new QThread();
//    this->voiceRecognizeThread->start();

    voiceRecognizeWorker.initialize();

    // done
    this->initialized = true;
}

void SonantManagerPrivate::record()
{
    DBG_LOG;
    if (!this->initialized) {
        ERR_LOG << "Manager is not initialized";
        return;
    }
    voiceRecognizeWorker.startRecord();
}

QStringList SonantManagerPrivate::getTranscription() const
{
    DBG_LOG;
    if (!this->initialized) {
        ERR_LOG << "Manager is not initialized";
        return QStringList();
    }

    return QStringList();
}

