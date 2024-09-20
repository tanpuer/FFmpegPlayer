#include "HYDemuxer.h"
#include "native_log.h"

HYDemuxer::HYDemuxer(std::shared_ptr<AssetManager> &assetManager,
                     const std::string &source) {
    this->assetManager = assetManager;
    this->source = source;
    this->assetManager->openVideo(source.c_str());
}

HYDemuxer::~HYDemuxer() {
    this->assetManager->closeVideo();
    avformat_free_context(fmt_ctx);
}

int HYDemuxer::prepare(JNIEnv *env, jobject javaPlayer) {
    if (onPreparedMethodId == nullptr) {
        auto clazz = env->FindClass("com/temple/ffmpegplayer/HYPlayer");
        onPreparedMethodId = env->GetMethodID(clazz, "onPrepared", "(JJII)V");
    }
    int ret = 0;
    fmt_ctx = avformat_alloc_context();
    avio_ctx = nullptr;
    uint8_t *avio_ctx_buffer = nullptr;
    size_t buffer_size, avio_ctx_buffer_size = 4096;
    avio_ctx_buffer = (uint8_t *) av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        return ret;
    }
    avio_ctx = avio_alloc_context(
            avio_ctx_buffer,
            avio_ctx_buffer_size,
            0,
            this,
            [](void *opaque, uint8_t *buf, int buf_size) -> int {
                auto demuxer = static_cast<HYDemuxer *>(opaque);
                return demuxer->assetManager->read(buf, buf_size);
            }, nullptr, [](void *opaque, int64_t offset, int whence) -> int64_t {
                auto demuxer = static_cast<HYDemuxer *>(opaque);
                return demuxer->assetManager->seek(offset, whence);
            });
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
    }
    fmt_ctx->pb = avio_ctx;
    ret = avformat_open_input(&fmt_ctx, source.c_str(), nullptr, nullptr);
    if (ret < 0) {
        ALOGE("Could not open source file:%s error:%d", source.c_str(), ret)
        return ret;
    }
    videoStreamIndex = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1,
                                           nullptr, 0);
    if (videoStreamIndex >= 0) {
        auto videoStream = fmt_ctx->streams[videoStreamIndex];
        auto r = videoStream->time_base;
        auto timeBase = r.num == 0 || r.den == 0 ? 0. : (double) r.num / (double) r.den;
        videoDuration = (int64_t) (videoStream->duration * 1000 * timeBase);
        videoWidth = videoStream->codecpar->width;
        videoHeight = videoStream->codecpar->height;
        ALOGD("HYDemuxer %s has video stream, index:%d duration:%ld width:%d height:%d bitRate:%ld",
              source.c_str(),
              videoStreamIndex, videoDuration, videoWidth, videoHeight,
              videoStream->codecpar->bit_rate
        )
    }
    audioStreamIndex = av_find_best_stream(fmt_ctx, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1,
                                           nullptr, 0);
    if (audioStreamIndex >= 0) {
        auto audioStream = fmt_ctx->streams[audioStreamIndex];
        auto r = audioStream->time_base;
        auto timeBase = r.num == 0 || r.den == 0 ? 0. : (double) r.num / (double) r.den;
        audioDuration = (int64_t) (audioStream->duration * 1000 * timeBase);
        ALOGD("HYDemuxer %s has audio stream, index:%d duration:%ld sampleRate:%d channels:%d",
              source.c_str(),
              audioStreamIndex, audioDuration, audioStream->codecpar->sample_rate,
              audioStream->codecpar->ch_layout.nb_channels)
    }
    ALOGD("start demuxer success")
    env->CallVoidMethod(javaPlayer, onPreparedMethodId, audioDuration, videoDuration, videoWidth,
                        videoHeight);
    return 0;
}

AVPacket *HYDemuxer::readOnePacket() {
    AVPacket *packet = av_packet_alloc();
    auto ret = av_read_frame(fmt_ctx, packet);
    if (ret != 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf));
//        ALOGD("av_read_frame error %s", buf)
        av_packet_free(&packet);
        return nullptr;
    }
    return packet;
}

AVStream *HYDemuxer::getVideoStream() {
    return fmt_ctx->streams[videoStreamIndex];
}

int HYDemuxer::getVideoStreamIndex() const {
    return videoStreamIndex;
}

AVStream *HYDemuxer::getAudioStream() {
    return fmt_ctx->streams[audioStreamIndex];
}

int HYDemuxer::getAudioStreamIndex() const {
    return audioStreamIndex;
}

bool HYDemuxer::seek(long pos) {
    avformat_flush(fmt_ctx);
    int re = 0;
    int64_t seekPos = getVideoStream()->duration * pos / videoDuration;
    re = av_seek_frame(fmt_ctx, videoStreamIndex, seekPos,
                       AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
    if (re >= 0) {
        ALOGD("seek video stream success %ld", pos)
        return true;
    }
    ALOGE("seek video stream error ! %d", AVERROR(re))
    seekPos = getAudioStream()->duration * pos / audioDuration;
    re = av_seek_frame(fmt_ctx, audioStreamIndex, seekPos,
                       AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
    if (re >= 0) {
        ALOGD("seek audio stream success %ld", pos)
        return true;
    }
    ALOGE("seek audio stream error ! %d", AVERROR(re))
    return false;
}
