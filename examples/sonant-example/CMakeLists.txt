cmake_minimum_required(VERSION 3.10)

# Set the project name
project(PrinterProject)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(/usr/local/include)

# Add the executable
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} /usr/local/lib/libSonant.so
                      /usr/local/lib/libwhisper.so)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Optionally, specify include directories if needed (not needed here since
# header is in the same directory) target_include_directories(PrinterApp PUBLIC
# ${CMAKE_CURRENT_SOURCE_DIR})

# Add any necessary compiler flags (if needed) set(CMAKE_CXX_FLAGS
# "${CMAKE_CXX_FLAGS} -pthread")

# Link libraries (if threading requires it, usually not needed since CMake
# handles it) target_link_libraries(PrinterApp pthread)
