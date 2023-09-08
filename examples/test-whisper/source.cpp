#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sndfile.h>
#include <SDL2/SDL.h>
#include <whisper.h>
#include "common.h"

//
static bool isRecording = false;

// for recording
static Sint16 *audioBuffer = nullptr;
static int audioBufferIndex = 0;
static int audioBufferLength = 0;
static unsigned long totalRecordedSamples = 0;

// function declaration
static void audio_callback(void* userdata, Uint8* stream, int len);
static void handleSignal(int signal);

static int startRecord();
#ifdef SAVE_TO_FILE
static int writeRecordToFile();
static int speechToTextFromFile(const char * audioPath);
#else
static int speechToTextFromBuffer();
#endif

int main()
{
    // Register the SIGINT signal handler
    signal(SIGINT, handleSignal);
    if (startRecord() != 0)
    {
        printf("Fail to record.\n");
        abort();
        return -1;
    }
#ifdef SAVE_TO_FILE
    printf("---------------------------------------------------------------\n");
    if (writeRecordToFile() != 0)
    {
        printf("Fail to write record to record.wav\n");
        return 1;
    }
    printf("---------------------------------------------------------------\n");
    if (speechToTextFromFile(AUDIO_RECORD) != 0)
    {
        printf("Fail to get text from speech.\n");
        return 1;
    }
#else
    printf("---------------------------------------------------------------\n");
    if (speechToTextFromBuffer() != 0)
    {
        printf("Fail to get text from speech.\n");
        return 1;
    }
#endif
    free(audioBuffer);

    return 0;
}

void audio_callback(void* userdata, Uint8* stream, int len)
{
    (void)userdata;
    if (isRecording)
    {
        // Ensure that the audio buffer has enough space
        if (audioBufferIndex + len / sizeof(Sint16) > (unsigned long) audioBufferLength)
        {
            // Resize the audio buffer as needed
            audioBufferLength = audioBufferIndex + len / sizeof(Sint16);
            audioBuffer = (Sint16*)realloc(audioBuffer, audioBufferLength * sizeof(Sint16));
            if (!audioBuffer)
            {
                printf("Failed to reallocate audio buffer.\n");
                return;
            }
        }
        // Copy audio data from the SDL stream to the audio buffer
        SDL_memcpy(audioBuffer + audioBufferIndex, stream, len);
        audioBufferIndex += len / sizeof(Sint16);

        // Update the total recorded samples
        totalRecordedSamples += len / sizeof(Sint16);
    }
}

void handleSignal(int signal)
{
    if (signal == SIGINT) {
        printf("Terminated.\n");
        abort();
    }
}

int startRecord()
{
    printf(">>> START RECORD\n");
    // Initialize SDL2
    printf("Initialize recorder.\n");
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return -1;
    }

    // Create an audio device for recording
    printf("Create audio device specs.\n");
    SDL_AudioSpec spec;
    spec.freq = SAMPLE_RATE;
    spec.format = SAMPLE_FORMAT;
    spec.channels = CHANNELS;
    spec.samples = BUFFER_SIZE;
    spec.callback = audio_callback;

    printf("Open audio device.\n");
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 1, &spec, NULL, 0);
    if (dev == 0)
    {
        printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        return -1;
    }

    // Prepare
    printf("Prepare audio buffer.\n");
    audioBuffer = (Sint16*)malloc(BUFFER_SIZE * sizeof(Sint16));
    if (!audioBuffer) {
        printf("Failed to allocate audio buffer.\n");
        return -1;
    }

    // Start recording using the audio callback
    printf("Start recording.\n");
    SDL_PauseAudioDevice(dev, 0);

    isRecording = true;
    SDL_Delay(RECORD_DURATION * 1000);


    // Stop recording
    printf("Stop recording.\n");
    isRecording = false;
    SDL_PauseAudioDevice(dev, 1);

    // Close the audio device
    printf("Close audio device.\n");
    SDL_CloseAudioDevice(dev);

    // Quit SDL2
    printf("Exit recorder.\n");
    SDL_Quit();
    return 0;
}

#ifdef SAVE_TO_FILE
int writeRecordToFile()
{
    printf("WRITE RECORD TO FILE\n");
    // Open the WAV file for writing
    printf("Saving the record.\n");
    SF_INFO sfinfo;
    sfinfo.samplerate = SAMPLE_RATE;
    sfinfo.channels = CHANNELS;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* sndfile = sf_open("record.wav", SFM_WRITE, &sfinfo);
    if (!sndfile)
    {
        printf("Failed to open WAV file for writing: %s\n", sf_strerror(NULL));
        return -1;
    }

    // Write the entire audioBuffer to the WAV file
    sf_write_short(sndfile, audioBuffer, totalRecordedSamples);

    // Close the WAV file
    sf_close(sndfile);

    printf("Done\n");
    return 0;
}

int speechToTextFromFile(const char *audioPath)
{
    printf("SPEAK TO TEXT FROM FILE\n");
    printf("Init whisper context.\n");
    const char* modelPath = TINY_MODEL;
    struct whisper_context* ctx = whisper_init_from_file(modelPath);
    if (!ctx) {
        printf("Failed to initialize Whisper context.\n");
        return 1;
    }

    printf("Read audio from file.\n");
    SF_INFO sfinfo;
    SNDFILE* sndfile = sf_open(audioPath, SFM_READ, &sfinfo);
    if (!sndfile) {
        printf("Failed to open audio file.\n");
        return 1;
    }

    // Read the audio data into a buffer
    long int numSamples = sfinfo.frames * sfinfo.channels;
    printf("Sample count: %ld\n", numSamples);

    float* audioData = (float*)malloc(sizeof(float) * numSamples);
    sf_readf_float(sndfile, audioData, numSamples);
    sf_close(sndfile);

    // Define parameters for transcription
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = NUM_PROCESS_THREAD;
    params.print_special = false; // Print special tokens like <SOT>, <EOT>, etc.
    params.print_progress = false; // Print progress information

    // Perform audio transcription
    int result = whisper_full(ctx, params, audioData, numSamples);
    if (result != 0) {
        printf("Failed to transcribe audio.\n");
        return 1;
    }

    // Retrieve and print the transcribed text
    int numSegments = whisper_full_n_segments(ctx);
    for (int i = 0; i < numSegments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        printf("Segment %d: %s\n", i + 1, text);
    }

    // Free the Whisper context
    whisper_free(ctx);
    free(audioData);
    return 0;
}
#else
int speechToTextFromBuffer()
{
    printf(">>> SPEAK TO TEXT FROM BUFFER\n");
    printf("Init whisper context.\n");
    const char* modelPath = TINY_MODEL;
    struct whisper_context* ctx = whisper_init_from_file(modelPath);
    if (!ctx) {
        printf("Failed to initialize Whisper context.\n");
        return 1;
    }

    // Read the audio data into a buffer
    float* audioData = (float*)malloc(sizeof(float) * totalRecordedSamples);
    if (!audioData) {
        printf("Failed to allocate memory for audioData.\n");
        return -1;
    }

    for (unsigned long i = 0; i < totalRecordedSamples; i++) {
        // Convert the 16-bit signed integer sample to a float and normalize it to [-1.0, 1.0]
        audioData[i] = (float)audioBuffer[i] / 32767.0f;
    }
    printf("Sample count: %ld\n", totalRecordedSamples);

    // Define parameters for transcription
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = 16;
    params.print_special = false; // Print special tokens like <SOT>, <EOT>, etc.
    params.print_progress = false; // Print progress information

    // Perform audio transcription
    int result = whisper_full(ctx, params, audioData, totalRecordedSamples);
    if (result != 0) {
        printf("Failed to transcribe audio.\n");
        return 1;
    }

    // Retrieve and print the transcribed text
    int numSegments = whisper_full_n_segments(ctx);
    for (int i = 0; i < numSegments; ++i) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        printf("%s\n", text);
    }

    // Free the Whisper context
    whisper_free(ctx);
    free(audioData);

    return 0;
}
#endif
