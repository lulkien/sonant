# Sonant

Sonant is a simple C++ wrapper that facilitates recording audio from the default microphone using ALSA and transcribing it into text using the [Whisper.cpp](https://github.com/ggerganov/whisper.cpp) library. 

This library abstracts the complexities of integrating ALSA for audio capture and Whisper for transcription, providing an easy-to-use API for developers.

## Dependencies
- ALSA
- Whisper.cpp
- CMake

## System Requirements
- Linux-based operating system (tested on Ubuntu and Arch Linux)
- C++14 or later

## Installation

### Install ALSA Library

#### Ubuntu
```bash
sudo apt-get install libasound2-dev
```

#### Arch Linux
```bash
sudo pacman -S alsa-lib
```

### Install Whisper.cpp from Source
```bash
git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
mkdir build
cd build
cmake ..
make -j
sudo make install
```

### Install Sonant from Source
```bash
git clone https://github.com/lulkien/sonant.git
cd sonant
mkdir build
cd build
cmake ..
make -j
sudo make install
```

## How to use
Include the header file, create a `Sonant` object, initialize it with your Whisper model, start the recorder, and handle the transcription in your callback.

```cpp
#include <iostream>
#include <sonant.h>

int main() {
    Sonant sonant;

    if (!sonant.initialize("/path/to/your/whisper/model")) {
        std::cerr << "Failed to initialize Sonant" << std::endl;
        return -1;
    }

    sonant.setTranscriptionCallback([](const std::string& transcription) {
        std::cout << "Transcription: " << transcription << std::endl;
    });

    if (!sonant.startRecorder()) {
        std::cerr << "Failed to start recording" << std::endl;
        return -1;
    }

    // Your application logic here

    sonant.terminate();
    return 0;
}
```

### Note:
Always call `terminate()` before your application exits to ensure proper cleanup.

## License
This project is released into the public domain under the [Unlicense](https://unlicense.org).
