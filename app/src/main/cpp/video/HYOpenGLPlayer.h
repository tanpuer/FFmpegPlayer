#pragma once

#include "EGLCore.h"
#include "jni.h"
#include "memory"
#include "IFilter.h"
#include "AssetManager.h"
#include "VideoData.h"
#include "queue"
#include "string"

class HYOpenGLPlayer {
public:

    HYOpenGLPlayer(JNIEnv *env, jobject javaAssets);

    ~HYOpenGLPlayer();

    void create(ANativeWindow *window);

    void change(int width, int height);

    void destroy();

    void doFrame(JNIEnv *env, jobject javaPlayer, long time);

    int pushVideoData(VideoData *data);

    void initFilter(VideoData *data);

    int64_t getVideoPts();

    void clearBuffer();

    void setTitle(const char* title);

private:

    std::unique_ptr<EGLCore> mEGLCore;

    std::unique_ptr<IFilter> mFilter;

    JNIEnv *env = nullptr;

    jobject globalAssetsRef = nullptr;

    std::shared_ptr<AssetManager> assetManager = nullptr;

    std::queue<VideoData *> datas;

    jmethodID updateVideoBufferInfoMethodId = nullptr;

    VideoData *currVideoData = nullptr;

    int viewWidth = 0, viewHeight = 0;

    std::atomic<int64_t> audioPts = 0L;

    std::string title = "";
};
