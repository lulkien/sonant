#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sndfile.h>
#include <SDL2/SDL.h>
#include <whisper.h>
#include "common.h"

inline void audio_callback(void* userdata, Uint8* stream, int len);
inline void handleSignal(int signal);

// Global variable to signal termination
bool isRunning = true;

int main_test()
{
    // Register the SIGINT signal handler
    signal(SIGINT, handleSignal);

#ifdef USING_SNDFILE
    // Initialize Whisper context with your model path
    const char* modelPath = "/storage/kienlh4ivi/MyProject/TestWhisper/vr-model/ggml-base.en.bin";
    struct whisper_context* ctx = whisper_init_from_file(modelPath);

    if (!ctx) {
        fprintf(stderr, "Failed to initialize Whisper context.\n");
        return 1;
    }

    const char* audioPath = "/storage/kienlh4ivi/MyProject/TestWhisper/samples/sample1.wav";

    SF_INFO sfinfo;
    SNDFILE* sndfile = sf_open(audioPath, SFM_READ, &sfinfo);
    if (!sndfile)
    {
        fprintf(stderr, "Failed to open audio file.\n");
        return 1;
    }

    // Read the audio data into a buffer
    int numSamples = sfinfo.frames * sfinfo.channels;
    float* audioData = (float*)malloc(sizeof(float) * numSamples);
    sf_readf_float(sndfile, audioData, numSamples);
    sf_close(sndfile);

    // Define parameters for transcription
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = 16;
    params.print_special = false; // Print special tokens like <SOT>, <EOT>, etc.
    params.print_progress = true; // Print progress information

    // Perform audio transcription
    int result = whisper_full(ctx, params, audioData, numSamples);

    if (result != 0)
    {
        fprintf(stderr, "Failed to transcribe audio.\n");
        return 1;
    }

    // Retrieve and print the transcribed text
    int numSegments = whisper_full_n_segments(ctx);
    for (int i = 0; i < numSegments; ++i)
    {
        const char* text = whisper_full_get_segment_text(ctx, i);
        printf("Segment %d: %s\n", i + 1, text);
    }

    // Free the Whisper context
    whisper_free(ctx);
#endif

#ifdef USING_SDL2
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    // Create an audio device for recording
    SDL_AudioSpec spec;
    spec.freq = SAMPLE_RATE;
    spec.format = SAMPLE_FORMAT;
    spec.channels = CHANNELS;
    spec.samples = BUFFER_SIZE;
    spec.callback = NULL;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 1, &spec, NULL, 0);
    if (dev == 0)
    {
        printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        return -1;
    }

    // Start recording
    SDL_PauseAudioDevice(dev, 0);

    // Define a buffer for audio data
    Uint8 buffer[BUFFER_SIZE * 2];

    // Process the audio data in a loop
    while (isRunning)
    {
        int len = SDL_DequeueAudio(dev, buffer, sizeof(buffer));
        if (len > 0)
        {
            Sint16 *samples = (Sint16 *)buffer;
            int num_samples = len / 2;
            int sum = 0;
            for (int i = 0; i < num_samples; i++)
            {
                sum += abs(samples[i]);
            }
            int avg = sum / num_samples;
            if (avg > THRESHOLD)
            {
                printf("Voice detected: %d\n", avg);
            }
        }
    }

    // Stop recording
    SDL_PauseAudioDevice(dev, 1);

    // Close the audio device
    SDL_CloseAudioDevice(dev);

    // Quit SDL2
    SDL_Quit();

    std::cout << "Exiting program." << std::endl;
#endif

    return 0;
}

inline void audio_callback(void* userdata, Uint8* stream, int len)
{
    (void) userdata;
    (void) stream;
    (void) len;
}

inline void handleSignal(int signal)
{
    if (signal == SIGINT) {
        isRunning = false;
    }
}
