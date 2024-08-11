#include "sonant.h"
#include "sonant_impl.h"
#include <string>
#include <functional>

Sonant::Sonant()
    : pImpl { std::make_unique<SonantImpl>() }
{

}

Sonant::~Sonant() {
    stopRecorder();
}

void Sonant::initialize() {
    pImpl->initialize();
}

bool Sonant::setModel(std::string path) {
    return pImpl->setModel(path);
}

std::string Sonant::getModel() const {
    return pImpl->getModel();
}

bool Sonant::startRecorder() {
    return pImpl->startRecorder();
}

bool Sonant::stopRecorder() {
    return pImpl->stopRecorder();
}

void Sonant::setTranscriptionCallback(std::function<void(std::string)> callback) {
    pImpl->setCallbackTranscriptionReady(callback);
}

void Sonant::terminate() {
    pImpl->terminate();
}
