#pragma once

#include <jni.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class HYVideoDecoder {

public:

    explicit HYVideoDecoder(AVStream *stream);

    ~HYVideoDecoder();

    void decodePacket(JNIEnv *env, jobject javaPlayer, AVPacket *packet);

    void flush();

private:

    AVCodecContext *codecContext = nullptr;

    jmethodID sendVideoFrameMethodId = nullptr;

    bool usingMediaCodec = false;

    double timeBase = 1.0;

};
