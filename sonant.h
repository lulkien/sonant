#ifndef SONANT_H
#define SONANT_H

#include <functional>
#include <memory>
#include <string>

/**
 * @brief Sonant config parameters
 */
struct SonantParams {
    /**
     * @brief Record threshold
     *
     * Record frame is valid and will be append
     * to record buffer if it has at least 1 sample
     * with absolute value above this threshold.
     * Default: 0.25
     */
    float recordThreshold = 0.25f;

    /**
     * @brief Pause recorder timer
     *
     * Recorder keeps record for a while after
     * last valid input (millisecond).
     * Default: 3000ms
     */
    uint64_t pauseRecorderDelay = 3000;

    /**
     * @brief Whisper API thread count
     *
     * Number of thread Whisper API use for
     * voice processing.
     * Default: 4
     */
    uint16_t sonantThreadCount = 4;
};

class SonantImpl;
class Sonant {
public:
    /**
     * @brief Sonant constructor
     */
    Sonant();

    /**
     * @brief Sonant destructor
     */
    virtual ~Sonant();

    /**
     * @brief Init Sonant object with default parameters
     *
     * This function must be called before using any other APIs
     * provided by this object. It initiates ALSA and Whisper.cpp
     * to ensures all of the APIs are ready to use.
     *
     * @return bool Result of initialize process.
     */
    bool initialize(const std::string &initModelPath);

    /**
     * @brief Init Sonant object with custom parameters
     *
     * This function must be called before using any other APIs
     * provided by this object. It initiates ALSA and Whisper.cpp
     * to ensures all of the APIs are ready to use.
     *
     * @return bool Result of initialize process.
     */
    bool initialize(const std::string &initModelPath,
                    const SonantParams &params);

    /**
     * @brief Change Whisper model path
     *
     * This function will try to set the model path for Whisper.
     * If it fails for any reasons, the old model will be used,
     * and the model path will not be changed.
     *
     * @param modelPath Whisper's model path.
     * @return Result of the request to change model.
     */
    bool requestChangeModel(const std::string &newModelPath);

    /**
     * @brief Start the recorder
     *
     * This function will start the recorder, make it listens
     * all the input from your default mic device.
     * It will return the result if the recorder started or not.
     *
     * @return Result of request start recorder.
     */
    bool startRecorder();

    /**
     * @brief Stop the recorder
     *
     * This function will stop the recorder, close opened mic
     * device and ignore every input.
     */
    void stopRecorder();

    /**
     * @brief Terminate the recorder
     *
     * This function will terminate the recorder.
     * The recorder will no longer able to be restart
     * after call this API.
     * This should be used onlu when you want to quit the
     * application. Other than that, please use stopRecorder()
     */
    void terminate();

    /**
     * @brief Set transcription callback
     *
     * This function set the callback, which will be called
     * every time Whisper complete processes a voice buffer.
     *
     * @param callback A functor with type void(std::string)
     */
    void setTranscriptionCallback(std::function<void(std::string)> callback);

private:
    std::unique_ptr<SonantImpl> pImpl;
};

#endif // !SONANT_H
