cmake_minimum_required(VERSION 3.22.1)

project("hyplayer")

set(JNI_LIBS_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs)

include_directories(
        ${JNI_LIBS_DIR}/ffmpeg/include
        src/main/cpp
        src/main/cpp/app
        src/main/cpp/audio
        src/main/cpp/base
        src/main/cpp/video
        src/main/cpp/demux
)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavcodec.so)
add_library(avdevice SHARED IMPORTED)
set_target_properties(avdevice
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavdevice.so)
add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavfilter.so)
add_library(avformat SHARED IMPORTED)
set_target_properties(avformat
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavformat.so)
add_library(avutil SHARED IMPORTED)
set_target_properties(avutil
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libavutil.so)
add_library(swresample SHARED IMPORTED)
set_target_properties(swresample
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libswresample.so)
add_library(swscale SHARED IMPORTED)
set_target_properties(swscale
        PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS_DIR}/${ANDROID_ABI}/libswscale.so)


aux_source_directory(src/main/cpp srcs)
aux_source_directory(src/main/cpp/app srcs)
aux_source_directory(src/main/cpp/audio srcs)
aux_source_directory(src/main/cpp/base srcs)
aux_source_directory(src/main/cpp/video srcs)
aux_source_directory(src/main/cpp/demux srcs)

add_library(hyplayer SHARED ${srcs})

find_library(log-lib log)
find_library(z-lib z)
find_library(android-lib android)

target_link_libraries(hyplayer

        avcodec
        avdevice
        avfilter
        avformat
        avutil
        swresample
        swscale

        android EGL GLESv3 jnigraphics OpenSLES
        ${log-lib} ${z-lib} ${android-lib})