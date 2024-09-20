#include <unistd.h>
#include "HYVideoDecoder.h"
#include "native_log.h"
#include "VideoData.h"

HYVideoDecoder::HYVideoDecoder(AVStream *stream) {
    const AVCodec *codec = nullptr;
    if (usingMediaCodec) {
        codec = avcodec_find_decoder_by_name("h264_mediacodec");
    }
    if (codec == nullptr) {
        codec = avcodec_find_decoder(stream->codecpar->codec_id);
    }
    if (!codec) {
        ALOGE("find decoder error for video")
        return;
    }
    codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0) {
        ALOGE("Failed to copy %s codec parameters to decoder context",
              av_get_media_type_string(AVMediaType::AVMEDIA_TYPE_VIDEO))
        return;
    }
    codecContext->thread_count = 2;
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        ALOGE("Failed to open %s codec", av_get_media_type_string(AVMediaType::AVMEDIA_TYPE_VIDEO))
        return;
    }
    auto r = stream->time_base;
    timeBase = r.num == 0 || r.den == 0 ? 0. : (double) r.num / (double) r.den;
}

HYVideoDecoder::~HYVideoDecoder() {
    if (codecContext) {
        avcodec_close(codecContext);
        avcodec_free_context(&codecContext);
    }
}

void HYVideoDecoder::decodePacket(JNIEnv *env, jobject javaPlayer, AVPacket *packet) {
    if (sendVideoFrameMethodId == nullptr) {
        auto clazz = env->FindClass("com/temple/ffmpegplayer/HYPlayer");
        sendVideoFrameMethodId = env->GetMethodID(clazz, "sendVideoFrame", "(J)V");
    }
    int ret = 0;
    ret = avcodec_send_packet(codecContext, packet);
    if (ret < 0) {
        ALOGE("Error sending a packet for decoding")
        return;
    }
    AVFrame *frame = av_frame_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            ALOGE("Error during decoding")
            break;
        }
        auto videoData = new VideoData();
        auto size =
                (frame->linesize[0] + frame->linesize[1] + frame->linesize[2]) *
                frame->height;
        videoData->videoWidth = frame->width;
        videoData->videoHeight = frame->height;
        videoData->height = frame->height;
        videoData->pts = frame->pts * 1000 * timeBase;

        videoData->lineSizeY = frame->linesize[0];
        auto ySize = frame->linesize[0] * frame->height;
        videoData->y = new unsigned char[ySize];
        memcpy(videoData->y, frame->data[0], ySize);

        if (frame->format == AVPixelFormat::AV_PIX_FMT_YUV420P) {
            videoData->type = VideoYUVType::YUV420P;
            videoData->lineSizeU = frame->linesize[1];
            auto uSize = frame->linesize[1] * frame->height / 2;
            videoData->u = new unsigned char[uSize];
            memcpy(videoData->u, frame->data[1], uSize);

            videoData->lineSizeV = frame->linesize[2];
            auto vSize = frame->linesize[2] * frame->height / 2;
            videoData->v = new unsigned char[vSize];
            memcpy(videoData->v, frame->data[2], vSize);
            env->CallVoidMethod(javaPlayer, sendVideoFrameMethodId,
                                reinterpret_cast<long >(videoData));
        } else if (frame->format == AVPixelFormat::AV_PIX_FMT_NV12 ||
                   frame->format == AVPixelFormat::AV_PIX_FMT_NV21) {
            videoData->type = frame->format == AVPixelFormat::AV_PIX_FMT_NV12 ? VideoYUVType::NV12
                                                                              : VideoYUVType::NV21;
            videoData->lineSizeU = frame->linesize[1];
            auto uSize = frame->linesize[1] * frame->height / 2;
            videoData->u = new unsigned char[uSize];
            memcpy(videoData->u, frame->data[1], uSize);
            env->CallVoidMethod(javaPlayer, sendVideoFrameMethodId,
                                reinterpret_cast<long >(videoData));
        } else {
            ALOGE("Todo: unsupported video format for render")
        }
    }
    av_frame_free(&frame);
}

void HYVideoDecoder::flush() {
    if (codecContext) {
        avcodec_flush_buffers(codecContext);
    }
}
