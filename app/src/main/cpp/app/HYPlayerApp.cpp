#include "HYPlayerApp.h"
#include "native_log.h"

HYPlayerApp::HYPlayerApp(JNIEnv *env, jobject javaAsset, jobject javaPlayer) {
    jniEnv = env;
    globalAssetRef = env->NewGlobalRef(javaAsset);
    assetManager = std::make_shared<AssetManager>(env, globalAssetRef);
}

HYPlayerApp::~HYPlayerApp() {
    jniEnv->DeleteGlobalRef(globalAssetRef);
}

/**
 * in demux thread
 */
void HYPlayerApp::setSource(JNIEnv *env, jobject javaPlayer, const char *source) {
    demuxer.reset(nullptr);
    audioDecoder.reset(nullptr);
    videoDecoder.reset(nullptr);
    this->source = source;
    demuxer = std::make_unique<HYDemuxer>(assetManager, source);
    if (demuxer->prepare(env, javaPlayer) == 0) {
        audioDecoder = std::make_unique<HYAudioDecoder>(demuxer->getAudioStream());
        videoDecoder = std::make_unique<HYVideoDecoder>(demuxer->getVideoStream());
    }
}

/**
 * in demux thread
 */
int HYPlayerApp::readOnePacket(JNIEnv *env, jobject javaPlayer) {
    if (sendAudioPacketMethodId == nullptr) {
        auto clazz = env->FindClass("com/temple/ffmpegplayer/HYPlayer");
        sendAudioPacketMethodId = env->GetMethodID(clazz, "sendAudioPacket", "(J)V");
        sendVideoPacketMethodId = env->GetMethodID(clazz, "sendVideoPacket", "(J)V");
        onDemuxCompleteMethodId = env->GetMethodID(clazz, "onDemuxComplete", "()V");
    }

    auto packet = demuxer->readOnePacket();
    if (packet == nullptr) {
        env->CallVoidMethod(javaPlayer, sendVideoPacketMethodId, reinterpret_cast<long>(nullptr));
        env->CallVoidMethod(javaPlayer, sendAudioPacketMethodId, reinterpret_cast<long>(nullptr));
        env->CallVoidMethod(javaPlayer, onDemuxCompleteMethodId);
        return -2;
    }
    if (packet->stream_index == demuxer->getVideoStreamIndex()) {
        env->CallVoidMethod(javaPlayer, sendVideoPacketMethodId, reinterpret_cast<long>(packet));
        return 2;
    } else if (packet->stream_index == demuxer->getAudioStreamIndex()) {
        env->CallVoidMethod(javaPlayer, sendAudioPacketMethodId, reinterpret_cast<long>(packet));
        return 1;
    } else {
        ALOGE("invalid AVPacket stream index")
        return -1;
    }
}

void HYPlayerApp::seek(long timeMills) {
    if (demuxer != nullptr) {
        demuxer->seek(timeMills);
    }
}

/**
 * in video thread
 */
void HYPlayerApp::decodeVideoPacket(JNIEnv *env, jobject javaPlayer, long packet) {
    if (videoDecoder == nullptr) {
        return;
    }
    auto avPacket = reinterpret_cast<AVPacket *>(packet);
    videoDecoder->decodePacket(env, javaPlayer, avPacket);
}

void HYPlayerApp::flushVideo() {
    if (videoDecoder != nullptr) {
        videoDecoder->flush();
    }
}

/**
 * in audio thread
 */
void HYPlayerApp::decodeAudioPacket(JNIEnv *env, jobject javaPlayer, long packet) {
    if (audioDecoder == nullptr) {
        return;
    }
    auto avPacket = reinterpret_cast<AVPacket *>(packet);
    audioDecoder->decodePacket(env, javaPlayer, avPacket);
}

void HYPlayerApp::flushAudio() {
    if (audioDecoder != nullptr) {
        audioDecoder->flush();
    }
}
