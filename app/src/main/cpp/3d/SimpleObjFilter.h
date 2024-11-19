#pragma once

#include "SkiaFilterWith3D.h"

typedef struct {
    GLuint vb_id;  // vertex buffer id
    int numTriangles;
    size_t material_id;
} DrawObject;

struct vec3 {
    float v[3];

    vec3() {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
    }
};

class SimpleObjFilter : public SkiaFilterWith3D {

public:

    SimpleObjFilter(std::shared_ptr<AssetManager> &assetManager,
                    VideoYUVType type);

    ~SimpleObjFilter();

    void draw3D() override;

    float getWidthRatio() override;

private:

    std::vector<DrawObject> gDrawObjects;

    std::map<std::string, GLuint> textures;

};
