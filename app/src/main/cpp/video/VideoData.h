#pragma once

#include <cstdint>

enum class VideoYUVType {
    YUV420P,
    NV12,
    NV21
};

struct VideoData {

    int64_t pts{};

    VideoYUVType type = VideoYUVType::YUV420P;

    unsigned char *y = nullptr;

    int lineSizeY = 0;

    unsigned char *u = nullptr;

    int lineSizeU = 0;

    unsigned char *v = nullptr;

    int lineSizeV = 0;

    int height = 0;

    int videoWidth, videoHeight = 0;

    ~VideoData() {
        delete[] y;
        delete[] u;
        delete[] v;
    }

};
