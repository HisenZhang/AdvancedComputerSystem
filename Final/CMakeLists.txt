cmake_minimum_required(VERSION 3.20)

project(DSP)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

add_executable(${PROJECT_NAME} main.cpp DSP.h ${CMAKE_CURRENT_SOURCE_DIR}/AudioFile/AudioFile.h)
add_executable(benchmark benchmark.cpp)
target_compile_definitions(benchmark PRIVATE BENCHMARK)
source_group(thirdparty/AudioFile AudioFile/AudioFile.h)

if(WIN32)
     target_link_libraries(${PROJECT_NAME} PRIVATE winmm.lib)
elseif(UNIX)
     find_package(PulseAudio)
     find_package(Threads REQUIRED)
     target_include_directories(${PROJECT_NAME} PRIVATE ${PULSEAUDIO_INCLUDE_DIR})
     target_link_libraries(${PROJECT_NAME} PRIVATE 
          ${PULSEAUDIO_LIBRARY} 
          ${PULSEAUDIO_SIMPLE_LIBRARY} 
          ${PULSEAUDIO_MAINLOOP_LIBRARY} 
          ${CMAKE_DL_LIBS} 
          Threads::Threads)
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} 
                PRIVATE /EHsc
                PRIVATE /arch:AVX)
else()
    target_compile_options(${PROJECT_NAME} 
                PRIVATE -mavx)
endif()

add_subdirectory(imgui)
add_subdirectory(nativefiledialog)

file(COPY        "${CMAKE_CURRENT_SOURCE_DIR}/data"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
