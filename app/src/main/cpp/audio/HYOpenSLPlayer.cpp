#include "HYOpenSLPlayer.h"
#include "native_log.h"

HYOpenSLPlayer::HYOpenSLPlayer() {

}

HYOpenSLPlayer::~HYOpenSLPlayer() {
    ALOGD("HYOpenSLPlayer release")
    //停止播放
    if (iplayer && (*iplayer)) {
        (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_STOPPED);
    }
    //清空缓冲队列
    if (pcmQue && (*pcmQue)) {
        (*pcmQue)->Clear;
    }
    //销毁player对象
    if (player && (*player)) {
        (*player)->Destroy(player);
    }
    //销毁混音器
    if (mix && (*mix)) {
        (*mix)->Destroy(mix);
    }
    if (engineSL && (*engineSL)) {
        (*engineSL)->Destroy(engineSL);
    }
    engineSL = nullptr;
    eng = nullptr;
    mix = nullptr;
    player = nullptr;
    iplayer = nullptr;
    pcmQue = nullptr;
    delete[] buf;
}

void HYOpenSLPlayer::init() {
    buf = new unsigned char[1024 * 1024];
    //1 创建引擎
    eng = createEngine();
    if (eng) {
        ALOGD("CreateSL success")
    } else {
        ALOGE("CreateSL failed!")
    }
    //2 创建混音器
    SLresult re = 0;
    re = (*eng)->CreateOutputMix(eng, &mix, 0, nullptr, nullptr);
    if (re != SL_RESULT_SUCCESS) {
        ALOGE("SL_RESULT_SUCCESS failed!")
        return;
    }
    re = (*mix)->Realize(mix, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) {
        ALOGE("(*mix)->Realize failed!")
        return;
    }
    SLDataLocator_OutputMix outmix = {SL_DATALOCATOR_OUTPUTMIX, mix};
    SLDataSink audioSink = {&outmix, 0};
    //3 配置音频信息
    //缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue que = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    //音频格式
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            (SLuint32) 2,//    声道数
            (SLuint32) 44100 * 1000,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN //字节序，小端
    };
    SLDataSource ds = {&que, &pcm};
    //4 创建播放器
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAY};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    re = (*eng)->CreateAudioPlayer(eng, &player, &ds, &audioSink,
                                   sizeof(ids) / sizeof(SLInterfaceID), ids, req);
    if (re != SL_RESULT_SUCCESS) {
        ALOGE("CreateHYOpenSLPlayer failed %d", re)
        return;
    } else {
        ALOGD("CreateHYOpenSLPlayer success!")
    }
    (*player)->Realize(player, SL_BOOLEAN_FALSE);
    //获取player接口
    re = (*player)->GetInterface(player, SL_IID_PLAY, &iplayer);
    if (re != SL_RESULT_SUCCESS) {
        ALOGE("GetInterface SL_IID_PLAY failed!")
        return;
    }
    re = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &pcmQue);
    if (re != SL_RESULT_SUCCESS) {
        ALOGE("GetInterface SL_IID_BUFFERQUEUE failed!")
        return;
    }
    re = (*player)->GetInterface(player, SL_IID_VOLUME, &volumeItf);
    if (re != SL_RESULT_SUCCESS) {
        ALOGE("GetInterface SL_IID_VOLUME failed!")
        return;
    }
    //设置回调函数，播放队列空调用
    (*pcmQue)->RegisterCallback(pcmQue, [](SLAndroidSimpleBufferQueueItf bf, void *context) {
        auto audioPlayer = (HYOpenSLPlayer *) context;
        std::lock_guard<std::mutex> lock(audioPlayer->mutex);
        if (audioPlayer->datas.empty()) {
            (*(audioPlayer->pcmQue))->Enqueue(audioPlayer->pcmQue, "", 1);
            return;
        }
        auto data = audioPlayer->datas.front();
        audioPlayer->datas.pop();
        audioPlayer->audioPts = data->pts;
        memcpy(audioPlayer->buf, data->data, data->size);
        (*(audioPlayer->pcmQue))->Enqueue(audioPlayer->pcmQue, audioPlayer->buf, data->size);
        delete data;
    }, this);
}

void HYOpenSLPlayer::start() {
    if (player == nullptr || *player == nullptr) {
        ALOGE("player is null , pls check")
        return;
    }
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_PLAYING);
    (*pcmQue)->Enqueue(pcmQue, "", 1);
}

void HYOpenSLPlayer::pause() {
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_PAUSED);
}

void HYOpenSLPlayer::stop() {
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_STOPPED);
}

SLEngineItf HYOpenSLPlayer::createEngine() {
    SLresult re;
    SLEngineItf en;
    re = slCreateEngine(&engineSL, 0, nullptr, 0, nullptr, nullptr);
    if (re != SL_RESULT_SUCCESS) return nullptr;
    re = (*engineSL)->Realize(engineSL, SL_BOOLEAN_FALSE);
    if (re != SL_RESULT_SUCCESS) return nullptr;
    re = (*engineSL)->GetInterface(engineSL, SL_IID_ENGINE, &en);
    if (re != SL_RESULT_SUCCESS) return nullptr;
    return en;
}

void HYOpenSLPlayer::pushFrame(AudioData *frame) {
    std::lock_guard<std::mutex> lock(mutex);
    datas.push(frame);
}

int64_t HYOpenSLPlayer::getAudioPts() {
    return audioPts;
}

void HYOpenSLPlayer::clearBuffer() {
    std::lock_guard<std::mutex> lock(mutex);
    while (!datas.empty()) {
        auto data = datas.front();
        datas.pop();
        delete data;
    }
}
