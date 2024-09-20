#pragma once

#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"
#include "SLES/OpenSLES_Platform.h"
#include "queue"
#include "AudioData.h"
#include "mutex"
#include "atomic"

class HYOpenSLPlayer {

public:

    HYOpenSLPlayer();

    HYOpenSLPlayer(HYOpenSLPlayer &) = delete;

    HYOpenSLPlayer(HYOpenSLPlayer &&) = delete;

    virtual ~HYOpenSLPlayer();

    void init();

    void start();

    void pause();

    void stop();

    void pushFrame(AudioData *audioData);

    std::queue<AudioData *> datas;

    int64_t getAudioPts();

    void clearBuffer();

private:

    unsigned char* buf = nullptr;

    SLObjectItf engineSL = nullptr;
    SLEngineItf eng = nullptr;
    SLObjectItf mix = nullptr;
    SLObjectItf player = nullptr;
    SLPlayItf iplayer = nullptr;
    SLAndroidSimpleBufferQueueItf pcmQue = nullptr;
    SLVolumeItf volumeItf = nullptr;

    SLEngineItf createEngine();

    std::mutex mutex;

    std::atomic<int64_t> audioPts = 0L;

};