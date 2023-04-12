cmake_minimum_required(VERSION 3.20)

project(DSP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

add_executable(${PROJECT_NAME} main.cpp DSP.cpp DSP.h ${CMAKE_CURRENT_SOURCE_DIR}/AudioFile/AudioFile.h)

if (MSVC)
    target_compile_options(${PROJECT_NAME} 
                PRIVATE /EHsc
                PRIVATE /arch:AVX)
else()
    target_compile_options(${PROJECT_NAME} 
                PRIVATE -mavx)
endif()

file(COPY        "${CMAKE_CURRENT_SOURCE_DIR}/data/test-audio.wav"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/data")
