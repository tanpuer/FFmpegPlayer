#include <jni.h>
#include <base/native_log.h>
#include <iterator>
#include "android/native_window_jni.h"
#include "HYPlayerApp.h"
#include "HYOpenSLPlayer.h"
#include "HYOpenGLPlayer.h"

extern "C" {
#include "libavcodec/jni.h"
}

const char *HYPlayer = "com/temple/ffmpegplayer/HYPlayer";

extern "C" JNIEXPORT jlong JNICALL
native_Init(JNIEnv *env, jobject instance, jobject javaAsset) {
    ALOGD("native_init")
    auto player = new HYPlayerApp(env, javaAsset, instance);
    return reinterpret_cast<long >(player);
}

extern "C" JNIEXPORT void JNICALL
native_SetSource(JNIEnv *env, jobject instance, jlong app, jstring source) {
    ALOGD("native_SetSource")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return;
    }
    auto path = env->GetStringUTFChars(source, nullptr);
    player->setSource(env, instance, path);
    env->ReleaseStringUTFChars(source, path);
}

extern "C" JNIEXPORT void JNICALL
native_SendAudioPacket(JNIEnv *env, jobject instance, jlong app, jlong packet) {
//    ALOGD("native_SendAudioPacket")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return;
    }
    player->decodeAudioPacket(env, instance, packet);
}

extern "C" JNIEXPORT void JNICALL
native_SendVideoPacket(JNIEnv *env, jobject instance, jlong app, jlong packet) {
//    ALOGD("HYOpenGLPlayer native_SendVideoPacket")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return;
    }
    player->decodeVideoPacket(env, instance, packet);
}

extern "C" JNIEXPORT jint JNICALL
native_ReadOnePacket(JNIEnv *env, jobject instance, jlong app) {
//    ALOGD("native_ReadOnePacket")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return -1;
    }
    return player->readOnePacket(env, instance);
}

extern "C" JNIEXPORT void JNICALL
native_Seek(JNIEnv *env, jobject instance, jlong app, jlong timeMills) {
    ALOGD("native_Seek")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return;
    }
    player->seek(timeMills);
}

extern "C" JNIEXPORT void JNICALL
native_FlushAudio(JNIEnv *env, jobject instance, jlong app, jlong timeMills) {
    ALOGD("native_FlushAudio")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return;
    }
    player->flushAudio();
}

extern "C" JNIEXPORT void JNICALL
native_FlushVideo(JNIEnv *env, jobject instance, jlong app, jlong timeMills) {
    ALOGD("native_FlushVideo")
    auto player = reinterpret_cast<HYPlayerApp *>(app);
    if (player == nullptr) {
        return;
    }
    player->flushVideo();
}

extern "C" JNIEXPORT void JNICALL
native_SendAudioData(JNIEnv *env, jobject instance, jlong app, jlong audioData) {
//    ALOGD("native_SendAudioData")
    auto audioPlayer = reinterpret_cast<HYOpenSLPlayer *>(app);
    if (audioPlayer == nullptr) {
        return;
    }
    audioPlayer->pushFrame(reinterpret_cast<AudioData *>(audioData));
}

extern "C" JNIEXPORT jlong JNICALL
native_CreateAudioPlayer(JNIEnv *env, jobject instance) {
    ALOGD("native_CreateAudioPlayer")
    auto audioPlayer = new HYOpenSLPlayer();
    audioPlayer->init();
    return reinterpret_cast<long>(audioPlayer);
}

extern "C" JNIEXPORT jlong JNICALL
native_CreateVideoPlayer(JNIEnv *env, jobject instance, jobject javaAssets) {
    ALOGD("native_CreateVideoPlayer")
    auto videoPlayer = new HYOpenGLPlayer(env, javaAssets);
    return reinterpret_cast<long >(videoPlayer);
}

extern "C" JNIEXPORT jint JNICALL
native_SendVideoData(JNIEnv *env, jobject instance, jlong app, jlong videoData) {
    auto videoPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (videoPlayer != nullptr) {
        return videoPlayer->pushVideoData(reinterpret_cast<VideoData *>(videoData));
    }
    return 0;
}

extern "C" JNIEXPORT void JNICALL
native_CreateSurface(JNIEnv *env, jobject instance, jlong app, jobject javaSurface) {
    ALOGD("native_CreateSurface")
    auto videoPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (videoPlayer != nullptr) {
        videoPlayer->create(ANativeWindow_fromSurface(env, javaSurface));
    }
}

extern "C" JNIEXPORT void JNICALL
native_ChangeSurface(JNIEnv *env, jobject instance, jlong app, jint width, jint height) {
    ALOGD("native_ChangeSurface")
    auto videoPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (videoPlayer != nullptr) {
        videoPlayer->change(width, height);
    }
}

extern "C" JNIEXPORT void JNICALL
native_DestroySurface(JNIEnv *env, jobject instance, jlong app, jobject javaSurface) {
    ALOGD("native_DestroySurface")
    auto videoPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (videoPlayer != nullptr) {
        videoPlayer->destroy();
    }
}

extern "C" JNIEXPORT void JNICALL
native_DoFrame(JNIEnv *env, jobject instance, jlong app, jlong time) {
    auto videoPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (videoPlayer != nullptr) {
        videoPlayer->doFrame(env, instance, time);
    }
}

extern "C" JNIEXPORT void JNICALL
native_ClearVideoBuffer(JNIEnv *env, jobject instance, jlong app, jlong time) {
    auto videoPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (videoPlayer != nullptr) {
        videoPlayer->clearBuffer();
    }
}

extern "C" JNIEXPORT jlong JNICALL
native_GetVideoPts(JNIEnv *env, jobject instance, jlong app, jlong time) {
    auto audioPlayer = reinterpret_cast<HYOpenGLPlayer *>(app);
    if (audioPlayer != nullptr) {
        return audioPlayer->getVideoPts();
    }
    return -1L;
}

extern "C" JNIEXPORT jlong JNICALL
native_GetAudioPts(JNIEnv *env, jobject instance, jlong app, jlong time) {
    auto audioPlayer = reinterpret_cast<HYOpenSLPlayer *>(app);
    if (audioPlayer != nullptr) {
        return audioPlayer->getAudioPts();
    }
    return -1L;
}

extern "C" JNIEXPORT void JNICALL
native_StartAudio(JNIEnv *env, jobject instance, jlong app) {
    auto audioPlayer = reinterpret_cast<HYOpenSLPlayer *>(app);
    if (audioPlayer != nullptr) {
        audioPlayer->start();
    }
}

extern "C" JNIEXPORT void JNICALL
native_PauseAudio(JNIEnv *env, jobject instance, jlong app) {
    auto audioPlayer = reinterpret_cast<HYOpenSLPlayer *>(app);
    if (audioPlayer != nullptr) {
        audioPlayer->pause();
    }
}

extern "C" JNIEXPORT void JNICALL
native_ClearAudioBuffer(JNIEnv *env, jobject instance, jlong app) {
    auto audioPlayer = reinterpret_cast<HYOpenSLPlayer *>(app);
    if (audioPlayer != nullptr) {
        audioPlayer->clearBuffer();
    }
}

static JNINativeMethod g_PlayerMethods[] = {
        {"nativeInit",              "(Landroid/content/res/AssetManager;)J", (void *) native_Init},
        {"nativeSetSource",         "(JLjava/lang/String;)V",                (void *) native_SetSource},
        {"nativeSendAudioPacket",   "(JJ)V",                                 (void *) native_SendAudioPacket},
        {"nativeSendVideoPacket",   "(JJ)V",                                 (void *) native_SendVideoPacket},
        {"nativeReadOnePacket",     "(J)I",                                  (void *) native_ReadOnePacket},
        {"nativeSeek",              "(JJ)V",                                 (void *) native_Seek},
        {"nativeFlushAudio",        "(J)V",                                  (void *) native_FlushAudio},
        {"nativeFlushVideo",        "(J)V",                                  (void *) native_FlushVideo},
        {"nativeSendAudioData",     "(JJ)V",                                 (void *) native_SendAudioData},
        {"nativeCreateAudioPlayer", "()J",                                   (void *) native_CreateAudioPlayer},
        {"nativeCreateVideoPlayer", "(Landroid/content/res/AssetManager;)J", (void *) native_CreateVideoPlayer},
        {"nativeSendVideoData",     "(JJ)I",                                 (void *) native_SendVideoData},
        {"nativeCreateSurface",     "(JLandroid/view/Surface;)V",            (void *) native_CreateSurface},
        {"nativeChangeSurface",     "(JII)V",                                (void *) native_ChangeSurface},
        {"nativeDestroySurface",    "(JLandroid/view/Surface;)V",            (void *) native_DestroySurface},
        {"nativeDoFrame",           "(JJ)V",                                 (void *) native_DoFrame},
        {"nativeClearVideoBuffer",  "(J)V",                                  (void *) native_ClearVideoBuffer},
        {"nativeGetAudioPts",       "(J)J",                                  (void *) native_GetAudioPts},
        {"nativeGetVideoPts",       "(J)J",                                  (void *) native_GetVideoPts},
        {"nativeStartAudio",        "(J)V",                                  (void *) native_StartAudio},
        {"nativePauseAudio",        "(J)V",                                  (void *) native_PauseAudio},
        {"nativeClearAudioBuffer",  "(J)V",                                  (void *) native_ClearAudioBuffer},
};

static int RegisterNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *nativeMethods,
                                 int methodNum) {
    ALOGD("RegisterNativeMethods start %s", className)
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        ALOGD("RegisterNativeMethods fail clazz == null")
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, nativeMethods, methodNum) < 0) {
        ALOGD("RegisterNativeMethods fail")
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static void UnRegisterNativeMethods(JNIEnv *env, const char *className) {
    ALOGD("UnRegisterNativeMethods start")
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        ALOGD("UnRegisterNativeMethods fail clazz == null")
    }
    env->UnregisterNatives(clazz);
}

extern "C" jint JNI_OnLoad(JavaVM *jvm, void *p) {
    JNIEnv *env = nullptr;
    if (jvm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    av_jni_set_java_vm(jvm, p);
    RegisterNativeMethods(env, HYPlayer, g_PlayerMethods, std::size(g_PlayerMethods));
    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *jvm, void *p) {
    ALOGD("JNI_OnUnload")
    JNIEnv *env = nullptr;
    if (jvm->GetEnv((void **) env, JNI_VERSION_1_6) != JNI_OK) {
        return;
    }
    UnRegisterNativeMethods(env, HYPlayer);
}