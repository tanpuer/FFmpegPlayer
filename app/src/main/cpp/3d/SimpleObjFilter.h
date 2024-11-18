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

private:

//    void LoadObjAndConvert(float bmin[3], float bmax[3],
//                           std::vector<DrawObject> *drawObjects,
//                           std::vector<tinyobj::material_t> &materials,
//                           std::map<std::string, GLuint> &textures);

//    void computeSmoothingShapes(tinyobj::attrib_t &inattrib,
//                                std::vector<tinyobj::shape_t> &inshapes,
//                                std::vector<tinyobj::shape_t> &outshapes,
//                                tinyobj::attrib_t &outattrib);
//
//    void computeAllSmoothingNormals(tinyobj::attrib_t &attrib,
//                                    std::vector<tinyobj::shape_t> &shapes);
//
//    bool hasSmoothingGroup(const tinyobj::shape_t &shape);
//
//    void computeSmoothingNormals(const tinyobj::attrib_t &attrib, const tinyobj::shape_t &shape,
//                                 std::map<int, vec3> &smoothVertexNormals);
//
//    void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]);
//
//    void computeSmoothingShape(tinyobj::attrib_t &inattrib, tinyobj::shape_t &inshape,
//                               std::vector<std::pair<unsigned int, unsigned int>> &sortedids,
//                               unsigned int idbegin, unsigned int idend,
//                               std::vector<tinyobj::shape_t> &outshapes,
//                               tinyobj::attrib_t &outattrib);
//
//    void normalizeVector(vec3 &v);

private:

    std::vector<DrawObject> gDrawObjects;

    std::map<std::string, GLuint> textures;

};
