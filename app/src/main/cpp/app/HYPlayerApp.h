#pragma once

#include <jni.h>
#include "string"
#include "memory"
#include "AssetManager.h"
#include "HYDemuxer.h"
#include "HYAudioDecoder.h"
#include "HYVideoDecoder.h"

class HYPlayerApp {

public:

    HYPlayerApp(JNIEnv *env, jobject javaAsset, jobject javaPlayer);

    ~HYPlayerApp();

    void setSource(JNIEnv *env, jobject javaPlayer, const char *source);

    int readOnePacket(JNIEnv *env, jobject javaPlayer);

    void decodeAudioPacket(JNIEnv *env, jobject javaPlayer, long packet);

    void decodeVideoPacket(JNIEnv *env, jobject javaPlayer, long packet);

    void seek(long timeMills);

    void flushAudio();

    void flushVideo();

private:

    JNIEnv *jniEnv = nullptr;

    jobject globalAssetRef = nullptr;

    jmethodID sendVideoPacketMethodId = nullptr;

    jmethodID sendAudioPacketMethodId = nullptr;

    jmethodID onDemuxCompleteMethodId = nullptr;

    std::string source;

    std::shared_ptr<AssetManager> assetManager = nullptr;

    std::unique_ptr<HYDemuxer> demuxer = nullptr;

    std::unique_ptr<HYAudioDecoder> audioDecoder = nullptr;

    std::unique_ptr<HYVideoDecoder> videoDecoder = nullptr;

};
