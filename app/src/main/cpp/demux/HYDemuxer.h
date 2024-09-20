#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
}

#include "string"
#include "jni.h"
#include "AssetManager.h"
#include "memory"

class HYDemuxer {

public:

    HYDemuxer(std::shared_ptr<AssetManager> &assetManager, const std::string &source);

    ~HYDemuxer();

    int prepare(JNIEnv *env, jobject javaPlayer);

    AVPacket *readOnePacket();

    AVStream *getVideoStream();

    int getVideoStreamIndex() const;

    AVStream *getAudioStream();

    int getAudioStreamIndex() const;

    bool seek(long pos);

private:

    AVFormatContext *fmt_ctx = nullptr;

    AVIOContext *avio_ctx = nullptr;

    int videoStreamIndex = -1;

    int64_t videoDuration = 0L;

    int videoWidth = 0, videoHeight = 0;

    int audioStreamIndex = -1;

    int64_t audioDuration = 0L;

private:

    std::shared_ptr<AssetManager> assetManager;

    std::string source;

    jmethodID onPreparedMethodId = nullptr;

};
