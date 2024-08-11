cmake_minimum_required(VERSION 3.12)

project(Sonant)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the SDL2 package
find_package(SDL2 REQUIRED)

# Include directories for required libraries (libwhisper)
include_directories(/usr/local/include)

# Source files
set(SOURCE_FILES sonant.cpp sonant_impl.cpp)

# Create share library
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

# Link the required libraries
target_link_libraries(${PROJECT_NAME} /usr/local/lib/libwhisper.so SDL2::SDL2)

# Include directories for header files
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Install the library and header file
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION /usr/local/lib)
install(FILES sonant.h DESTINATION /usr/local/include)
