#ifndef COMMON_H
#define COMMON_H

// Constants for audio settings
#define SAMPLE_RATE 16000
#define SAMPLE_FORMAT AUDIO_S16LSB
#define CHANNELS 1
#define BUFFER_SIZE 4096
#define THRESHOLD 1000

#define RECORD_DURATION 5

#define TINY_MODEL      "/storage/kienlh4ivi/MyProject/sonant/vr-model/ggml-tiny.en.bin"
#define BASE_MODEL      "/storage/kienlh4ivi/MyProject/sonant/vr-model/ggml-base.en.bin"
#define MEDIUM_MODEL    "/storage/kienlh4ivi/MyProject/sonant/vr-model/ggml-medium.en.bin"

#define AUDIO_RECORD    "/home/kienlh4ivi/working/MyProject/sonant/examples/test-whisper/build/record.wav"
#define SAMPLE_RECORD   "/storage/kienlh4ivi/MyProject/sonant/samples/sample1.wav"

#define DBG_LOG         qDebug().noquote() << "[" << __PRETTY_FUNCTION__ << "]"

#endif // COMMON_H
