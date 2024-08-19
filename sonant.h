#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <functional>

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
     * @brief Initialize Sonant object
     *
     * This function must be called before using any other APIs
     * provided by this object. It initiates SDL2 and Whisper.cpp
     * to ensures all of the APIs are ready to use.
     *
     * @return Result of initialize process.
     */
    bool initialize(const std::string& initModelPath);

    /**
     * @brief Set Whisper model path
     *
     * This function will try to set the model path for Whisper.
     * If it fails for any reasons, the old model will be used,
     * and the model path will not be changed.
     *
     * @param modelPath Whisper's model path.
     * @return Result of the request to change model.
     */
    bool setModel(const std::string& modelPath);

    /**
     * @brief Get current model path is being used.
     *
     * This function will return the current model path
     * used by Whisper.
     *
     * @return Current model path.
     */
    std::string getModel() const;

    /**
     * @brief Set record threshold
     *
     * This function will set a threshold for the recorder.
     * If a record buffer has at least 1 value above this,
     * this buffer will be insert into final record.
     * Range: (0, 1)
     *
     * @param threshold Threshold for recorder.
     */
    void setRecordThreshold(float_t threshold);

    /**
     * @brief Get record threshold
     *
     * This function will return current threshold value
     * of the recorder.
     *
     * @return Threshold of the recorder.
     */
    float_t getRecordThreshold() const;

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
