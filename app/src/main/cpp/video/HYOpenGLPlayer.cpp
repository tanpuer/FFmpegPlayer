#include "HYOpenGLPlayer.h"
#include "YUV420PFilter.h"
#include "native_log.h"
#include "NV12Filter.h"
#include "NV21Filter.h"
#include "SkiaFilter.h"
#include "SkiaFilterWith3D.h"
#include "Simple3DFilter.h"
#include "SimpleObjFilter.h"


HYOpenGLPlayer::HYOpenGLPlayer(JNIEnv *env, jobject javaAssets) {
    this->env = env;
    globalAssetsRef = env->NewGlobalRef(javaAssets);
    assetManager = std::make_shared<AssetManager>(env, globalAssetsRef);
}

HYOpenGLPlayer::~HYOpenGLPlayer() {
    env->DeleteGlobalRef(globalAssetsRef);
    delete currVideoData;
    currVideoData = nullptr;
}

void HYOpenGLPlayer::create(ANativeWindow *window) {
    mEGLCore = std::make_unique<EGLCore>();
    mEGLCore->createGLEnv(nullptr, window, 0, 0, false);
    mEGLCore->makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void HYOpenGLPlayer::change(int width, int height) {
    glViewport(0, 0, width, height);
    viewWidth = width;
    viewHeight = height;
    if (mFilter) {
        mFilter->setWindowSize(viewWidth, viewHeight);
    }
}

void HYOpenGLPlayer::destroy() {
    mFilter.reset(nullptr);
    mEGLCore.reset(nullptr);
}

void HYOpenGLPlayer::doFrame(JNIEnv *env, jobject javaPlayer, long time) {
    if (updateVideoBufferInfoMethodId == nullptr) {
        auto clazz = env->FindClass("com/temple/ffmpegplayer/HYPlayer");
        updateVideoBufferInfoMethodId = env->GetMethodID(clazz, "updateVideoBufferInfo", "(I)V");
    }
    if (datas.empty()) {
//        ALOGD("HYOpenGLPlayer render datas is empty")
        return;
    }
    //surface销毁后默认继续播放
    if (mEGLCore != nullptr) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(1.0, 1.0, 1.0, 1.0);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
    }
    auto data = datas.front();
    //Todo 优化视频向音屏同步
    if (data->pts <= time) {
        datas.pop();
        delete currVideoData;
        currVideoData = data;
    }
    ALOGD("render video-pts:%ld audio-pts:%ld videoBuffer%ld", data->pts, time, datas.size())
    if (mEGLCore != nullptr) {
        initFilter(currVideoData);
        mFilter->render(currVideoData);
        mEGLCore->swapBuffer();
    }
    env->CallVoidMethod(javaPlayer, updateVideoBufferInfoMethodId, static_cast<int>(datas.size()));
}

void HYOpenGLPlayer::initFilter(VideoData *data) {
    if (mFilter != nullptr) {
        return;
    }
//    mFilter = std::make_unique<SkiaFilter>(assetManager, data->type);
//    mFilter = std::make_unique<Simple3DFilter>(assetManager, data->type);
    mFilter = std::make_unique<SimpleObjFilter>(assetManager, data->type);
//    if (data->type == VideoYUVType::YUV420P) {
//        mFilter = std::make_unique<YUV420PFilter>(assetManager);
//    } else if (data->type == VideoYUVType::NV12) {
//        mFilter = std::make_unique<NV12Filter>(assetManager);
//    } else if (data->type == VideoYUVType::NV21) {
//        mFilter = std::make_unique<NV21Filter>(assetManager);
//    }
    mFilter->setWindowSize(viewWidth, viewHeight);
    mFilter->setTitle(title.c_str());
}

int HYOpenGLPlayer::pushVideoData(VideoData *data) {
    datas.push(data);
    return datas.size();
}

int64_t HYOpenGLPlayer::getVideoPts() {
    if (currVideoData == nullptr) {
        return 0L;
    }
    return currVideoData->pts;
}

void HYOpenGLPlayer::clearBuffer() {
    while (!datas.empty()) {
        auto data = datas.front();
        datas.pop();
        delete data;
    }
}

void HYOpenGLPlayer::setTitle(const char *title) {
    this->title = title;
    if (mFilter != nullptr) {
        mFilter->setTitle(title);
    }
}
