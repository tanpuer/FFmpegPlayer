#pragma once

#include <jni.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
}

class HYAudioDecoder {

public:

    explicit HYAudioDecoder(AVStream *stream);

    ~HYAudioDecoder();

    void decodePacket(JNIEnv *env, jobject javaPlayer, AVPacket *packet);

    void flush();

private:

    AVCodecContext *codecContext = nullptr;

    jmethodID sendAudioFrameMethodId = nullptr;

    SwrContext *swrContext = nullptr;

    double timeBase = 1.0;

};
