#include "sonant.h"
#include "sonant_impl.h"

Sonant::Sonant() : pImpl{std::make_unique<SonantImpl>()} {}

Sonant::~Sonant() { stopRecorder(); }

bool Sonant::initialize(const std::string &initModelPath) {
    SonantParams params;
    return pImpl->initialize(initModelPath, params);
}

bool Sonant::initialize(const std::string &initModelPath, const SonantParams &params) {
    return pImpl->initialize(initModelPath, params);
}

bool Sonant::requestChangeModel(const std::string &newModelPath) {
    return pImpl->requestChangeModel(newModelPath);
}

bool Sonant::startRecorder() { return pImpl->startRecorder(); }

void Sonant::stopRecorder() { pImpl->stopRecorder(); }

void Sonant::terminate() { pImpl->terminate(); }

void Sonant::setTranscriptionCallback(CallbackFuncType callback) {
    pImpl->setCallbackTranscriptionReady(callback);
}
