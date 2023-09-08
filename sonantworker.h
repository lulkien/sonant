#ifndef SONANTWORKER_H
#define SONANTWORKER_H

#include <QObject>

class SonantWorker : public QObject
{
    Q_OBJECT
public:
    explicit SonantWorker(QObject *parent = nullptr);
    ~SonantWorker();

signals:

};

#endif // SONANTWORKER_H
