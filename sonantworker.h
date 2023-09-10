#ifndef SONANTWORKER_H
#define SONANTWORKER_H

#include <QObject>
#include <SDL2/SDL.h>
#include <whisper.h>

class SonantWorker : public QObject
{
    Q_OBJECT
public:
    explicit SonantWorker(QObject *parent = nullptr);
    ~SonantWorker();
    void initialize();
    void setWhisperModel(QString modelPath);
    QStringList getLatestTranscription() const;

public slots:
    void onRequestRecord();

private slots:
    int startRecord();
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

    // SDL
    SDL_AudioDeviceID m_audioDevice;
    SDL_AudioSpec m_recordSpecs;

    // Whisper
    whisper_context *m_whisperCtx;
    QString m_whisperModel;
    QStringList m_transcription;
};

#endif // SONANTWORKER_H
