#pragma once


#include "SkiaFilterWith3D.h"

static GLfloat vertices[] = {
        -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,

        -1.0f, -1.0f,  1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,

        1.0f, -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
        1.0f,  1.0f, -1.0f,  0.0f, 1.0f,


        -1.0f, -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f, -1.0f,  1.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,  0.0f, 1.0f,


        -1.0f,  1.0f, -1.0f,  0.0f, 0.0f,
        1.0f,  1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.0f, 1.0f,

};

static GLushort indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8,10,11,
        12,13,14,12,14,15,
        16,17,18,16,18,19,
        20,21,22,20,22,23
};

class Simple3DFilter : public SkiaFilterWith3D {

public:

    Simple3DFilter(std::shared_ptr<AssetManager> &assetManager,
                   VideoYUVType type);

    ~Simple3DFilter();

    void draw3D() override;

};
