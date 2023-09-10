#ifndef COMMON_H
#define COMMON_H

// Constants for audio settings
#define SAMPLE_RATE 16000
#define SAMPLE_FORMAT AUDIO_S16LSB
#define CHANNELS 1
#define BUFFER_SIZE 4096
#define THRESHOLD 1000
#define RECORD_DURATION 2
#define NUM_PROCESS_THREAD    6

#define MAX_F_SINT16    32767.0f

#define TINY_MODEL      "/home/ark/Working/Qt/sonant/models/ggml-tiny.en.bin"
#define BASE_MODEL      "/home/ark/Working/Qt/sonant/models/ggml-base.en.bin"
#define MEDIUM_MODEL    ""

#define AUDIO_RECORD    "/home/ark/Working/Qt/sonant/examples/test-whisper/build/record.wav"
#define SAMPLE_RECORD   "/home/ark/Working/Qt/sonant/samples/sample1.wav"

#define DBG_LOG         qDebug().noquote() << "[" << __PRETTY_FUNCTION__ << "]"
#define INF_LOG         qInfo().noquote() << "[" << __PRETTY_FUNCTION__ << "]"
#define WRN_LOG         qWarning().noquote() << "[" << __PRETTY_FUNCTION__ << "]"
#define ERR_LOG         qCritical().noquote() << "[" << __PRETTY_FUNCTION__ << "]"

#endif // COMMON_H
