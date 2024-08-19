#include "sonant.h"
#include "sonant_impl.h"
#include <cmath>
#include <string>
#include <functional>

Sonant::Sonant()
    : pImpl { std::make_unique<SonantImpl>() }
{

}

Sonant::~Sonant() {
    stopRecorder();
}

bool Sonant::initialize(const std::string& initModelPath) {
    return pImpl->initialize(initModelPath);
}

bool Sonant::setModel(const std::string& modelPath) {
    return pImpl->setModel(modelPath);
}

std::string Sonant::getModel() const {
    return pImpl->getModel();
}

void Sonant::setRecordThreshold(float_t threshold) {
    pImpl->setRecordThreshold(threshold);
}

float_t Sonant::getRecordThreshold() const {
    return pImpl->getRecordThreshold();
}

bool Sonant::startRecorder() {
    return pImpl->startRecorder();
}

void Sonant::stopRecorder() {
    pImpl->stopRecorder();
}

void Sonant::terminate() {
    pImpl->terminate();
}

void Sonant::setTranscriptionCallback(std::function<void(std::string)> callback) {
    pImpl->setCallbackTranscriptionReady(callback);
}
