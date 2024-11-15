#pragma once

#include "SkiaFilterWith3D.h"

class SimpleObjFilter : public SkiaFilterWith3D {

public:

    typedef struct {
        GLuint vb_id;  // vertex buffer id
        int numTriangles;
        size_t material_id;
    } DrawObject;

public:

    SimpleObjFilter(std::shared_ptr<AssetManager> &assetManager,
                    VideoYUVType type);

    ~SimpleObjFilter();

    void draw3D() override;

};
