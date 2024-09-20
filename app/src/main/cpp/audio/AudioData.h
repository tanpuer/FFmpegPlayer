#pragma once

#include <cstdint>

struct AudioData {

    int64_t pts;

    unsigned char *data;

    int size;

    ~AudioData() {
        delete[] data;
        data = nullptr;
    }

};
