#ifndef SONANTMANAGER_H
#define SONANTMANAGER_H

#include <QtCore/qscopedpointer.h>
#include <QtCore/qstring.h>

class SonantManagerPrivate;
class Q_DECL_EXPORT SonantManager
{
    Q_DECLARE_PRIVATE(SonantManager)
public:
    SonantManager();
    virtual ~SonantManager();

    void initialize();
    void startRecording();
    QStringList getTranscription();

private:
    QScopedPointer<SonantManagerPrivate> d_ptr;
};

#endif // SONANTMANAGER_H
