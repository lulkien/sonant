cmake_minimum_required(VERSION 3.12)
project(SonantLibrary)

set(CMAKE_CXX_STANDARD 11)

# Find the Qt5 package
find_package(Qt5 COMPONENTS Core REQUIRED)

# Include directories for required libraries (libwhisper and libSDL2)
include_directories(/usr/local/include)
include_directories(/usr/include/SDL2)

# Source files
set(SOURCE_FILES
    sonantmanager.cpp
    sonantworker.cpp
)

# Add MOC step for Qt-specific headers
set(QT_HEADERS
    sonantmanager.h
    sonantmanager_p.h
    sonantworker.h
)
QT5_WRAP_CPP(QT_MOC_HEADERS ${QT_HEADERS})

# Create the shared library
add_library(sonant SHARED ${SOURCE_FILES} ${QT_MOC_HEADERS})

# Link the required libraries
target_link_libraries(sonant
    /usr/local/lib/libwhisper.so
    /usr/lib/libSDL2.so
    Qt5::Core
)

# Include directories for header files
target_include_directories(sonant PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Install the library and header file
install(TARGETS sonant
    LIBRARY DESTINATION /usr/local/lib
)

install(FILES sonantmanager.h
    DESTINATION /usr/local/include
)

