#pragma once

#include "SkiaFilterWith3D.h"

class SimpleObjFilter : public SkiaFilterWith3D {

public:

    SimpleObjFilter(std::shared_ptr<AssetManager> &assetManager,
                    VideoYUVType type);

    ~SimpleObjFilter();

    void draw3D() override;

};
