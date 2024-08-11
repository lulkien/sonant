#ifndef SONANTMANAGER_H
#define SONANTMANAGER_H

#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>
#include <QObject>

class SonantManagerPrivate;
class Q_DECL_EXPORT SonantManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SonantManager)
public:
    SonantManager();
    virtual ~SonantManager();

    void setModel(const QString &modelPath);
    void initialize();
    void record();
    QStringList transcription();

signals:
    void recordCompleted();
    void transcriptionReady();

private:
    QScopedPointer<SonantManagerPrivate> d_ptr;
};

#endif // SONANTMANAGER_H
