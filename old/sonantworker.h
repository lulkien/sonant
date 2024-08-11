#ifndef SONANTWORKER_H
#define SONANTWORKER_H

#include <SDL2/SDL.h>
#include <whisper.h>
#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>

class SonantWorker : public QObject
{
    Q_OBJECT
public:
    explicit SonantWorker(QObject *parent = nullptr);
    ~SonantWorker();
    QStringList getLatestTranscription() const;

public slots:
    void onRequestChangeModel(const QString &modelPath);
    void onRequestInitialize();
    void onRequestRecord();

private slots:
    void initialize();
    void setModel(const QString &modelPath);
    int record();
    int processSpeech();

signals:
    void recordCompleted();
    void transcriptionReady();

public:
    // SDL
    Sint16 *recordedBuffer;
    Uint32 recordedSampleCount;
    Uint32 recordBufferSize;

private:
    bool m_initialized;
    bool m_recording;
    bool m_processing;

    // Threading
    QMutex m_lock;

    // SDL
    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_recordSpecs;

    // Whisper
    whisper_context *m_whisperCtx;
    QString m_whisperModel;
    QStringList m_transcription;
};

#endif // SONANTWORKER_H
