#include "HYAudioDecoder.h"
#include "native_log.h"
#include "AudioData.h"

HYAudioDecoder::HYAudioDecoder(AVStream *stream) {
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        ALOGE("find decoder error for audio")
        return;
    }
    codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0) {
        ALOGE("Failed to copy %s codec parameters to decoder context",
              av_get_media_type_string(AVMediaType::AVMEDIA_TYPE_AUDIO))
        return;
    }
    codecContext->thread_count = 2;
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        ALOGE("Failed to open %s codec", av_get_media_type_string(AVMediaType::AVMEDIA_TYPE_AUDIO))
        return;
    }
    AVChannelLayout src_ch_layout = stream->codecpar->ch_layout;
    AVChannelLayout dst_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    swrContext = swr_alloc();
    av_opt_set_chlayout(swrContext, "in_chlayout", &src_ch_layout, 0);
    av_opt_set_chlayout(swrContext, "out_chlayout", &dst_ch_layout, 0);
    av_opt_set_int(swrContext, "in_sample_rate", stream->codecpar->sample_rate, 0);
    av_opt_set_int(swrContext, "out_sample_rate", 44100, 0);
    auto format = (AVSampleFormat) stream->codecpar->format;
    if (format == AVSampleFormat::AV_SAMPLE_FMT_NONE) {
        format = AV_SAMPLE_FMT_FLTP;
    }
    av_opt_set_sample_fmt(swrContext, "in_sample_fmt", format, 0);
    av_opt_set_sample_fmt(swrContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    auto ret = swr_init(swrContext);
    if (ret != 0) {
        ALOGE("swr_init failed!")
        return;
    }
    ALOGD("swr_init success!")
    auto r = stream->time_base;
    timeBase = r.num == 0 || r.den == 0 ? 0. : (double) r.num / (double) r.den;
}

HYAudioDecoder::~HYAudioDecoder() {
    if (codecContext) {
        avcodec_close(codecContext);
        avcodec_free_context(&codecContext);
    }
    if (swrContext) {
        swr_free(&swrContext);
    }
}

void HYAudioDecoder::decodePacket(JNIEnv *env, jobject javaPlayer, AVPacket *packet) {
    if (sendAudioFrameMethodId == nullptr) {
        auto clazz = env->FindClass("com/temple/ffmpegplayer/HYPlayer");
        sendAudioFrameMethodId = env->GetMethodID(clazz, "sendAudioFrame", "(J)V");
    }
    int ret = 0;
    ret = avcodec_send_packet(codecContext, packet);
    if (ret < 0) {
        ALOGE("Error submitting a packet for decoding (%s)", av_err2str(ret));
        return;
    }
    AVFrame *frame = av_frame_alloc();
    while (ret >= 0) {
        ret = avcodec_receive_frame(codecContext, frame);
        if (ret == 0) {
            int size = 2 * frame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            uint8_t *output;
            int out_samples = av_rescale_rnd(
                    swr_get_delay(swrContext, frame->sample_rate) + frame->nb_samples, 44100,
                    frame->sample_rate, AV_ROUND_UP);
            av_samples_alloc(&output, nullptr, 2, out_samples, AV_SAMPLE_FMT_S16, 0);
            int len = swr_convert(swrContext, &output, out_samples,
                                  const_cast<const uint8_t **>(frame->data), frame->nb_samples);
            if (len <= 0) {
                ALOGE("resample error")
                delete[] output;
                continue;
            }
            auto audioData = new AudioData();
            audioData->size = len * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            audioData->pts = frame->pts * 1000 * timeBase;
            audioData->data = output;
            env->CallVoidMethod(javaPlayer, sendAudioFrameMethodId,
                                reinterpret_cast<long >(audioData));
        }
    }
    av_frame_free(&frame);
}

void HYAudioDecoder::flush() {
    if (codecContext) {
        avcodec_flush_buffers(codecContext);
    }
}
