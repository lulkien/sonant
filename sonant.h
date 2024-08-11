#include <memory>
#include <string>
#include <functional>

class SonantImpl;
class Sonant {
public:
    Sonant();
    virtual ~Sonant();

    void initialize();

    bool setModel(std::string path);
    std::string getModel() const;

    bool startRecorder();

    bool stopRecorder();

    void setTranscriptionCallback(std::function<void(std::string)> callback);

    void terminate();

private:
    std::unique_ptr<SonantImpl> pImpl;
};
