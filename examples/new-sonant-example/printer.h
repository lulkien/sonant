#ifndef PRINTER_H
#define PRINTER_H

#include <memory>

class Printer {
public:
    Printer();
    ~Printer();

    void startPrint();
    void stopPrint();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // PRINTER_H
