#include "printer.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>

class Printer::Impl {
public:
    void startPrint() {
        running = true;
        workerThread = std::thread([this]() {
            while (running) {
                std::cout << "Hello" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    void stopPrint() {
        running = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }

    ~Impl() {
        stopPrint();
    }

private:
    std::thread workerThread;
    std::atomic<bool> running{false};
};

Printer::Printer() : pImpl(std::make_unique<Impl>()) {}

Printer::~Printer() = default;

void Printer::startPrint() {
    pImpl->startPrint();
}

void Printer::stopPrint() {
    pImpl->stopPrint();
}

