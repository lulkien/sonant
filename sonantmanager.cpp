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

QString SonantManager::getTranscription()
{
    DBG_LOG;
    Q_D(SonantManager);
    return d->transcription();
}

SonantManagerPrivate::SonantManagerPrivate(SonantManager *q_ptr)
{
    DBG_LOG;
    this->q_ptr = q_ptr;
    this->initialized = false;
    this->processing = false;
    this->voiceRecognizeThread = nullptr;
}

SonantManagerPrivate::~SonantManagerPrivate()
{
    DBG_LOG;
}

void SonantManagerPrivate::initialize()
{
    DBG_LOG;
    this->voiceRecognizeThread = new QThread();
    this->voiceRecognizeThread->start();

    this->initialized = true;
}

void SonantManagerPrivate::fatality(const QString &errorString)
{
    DBG_LOG << errorString;
    abort();
}

void SonantManagerPrivate::record()
{
    DBG_LOG;
    if (!this->initialized) {
        fatality("Manager is not initialized");
    }
    DBG_LOG << "OK";
}

QString SonantManagerPrivate::transcription()
{
    DBG_LOG;
    if (!this->initialized) {
        fatality("Manager is not initialized");
    }

    if (this->processing) {
        DBG_LOG << "Speech data is processing, try again...";
        return QString();
    }

    return QString();
}
