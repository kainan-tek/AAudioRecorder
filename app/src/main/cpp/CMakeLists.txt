# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name
project("aaudiorecorder")

# Creates and names a library, sets it as either STATIC or SHARED,
# and provides the relative paths to its source code.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # 全新AAudio录音器实现文件
        aaudio_recorder.cpp
        )

# Specifies libraries CMake should link to your target library.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        android
        aaudio
        log
        )