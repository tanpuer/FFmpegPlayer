#include "SimpleObjFilter.h"

#define TINYOBJLOADER_IMPLEMENTATION

#include "obj/tiny_obj_loader.h"

SimpleObjFilter::SimpleObjFilter(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type)
        : SkiaFilterWith3D(assetManager, type) {

    tinyobj::ObjReader reader;

    auto objStr = assetManager->readImage("tv/tv.obj");
    auto mtlStr = assetManager->readImage("tv/tv.mtl");
    if (!reader.ParseFromString(objStr->content, mtlStr->content)) {
        if (!reader.Error().empty()) {
            ALOGE("TinyObjReader: %s", reader.Error().c_str())
        }
        return;
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();
    auto &materials = reader.GetMaterials();

// Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                // Optional: vertex colors
                tinyobj::real_t red = attrib.colors[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t green = attrib.colors[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t blue = attrib.colors[3 * size_t(idx.vertex_index) + 2];
            }
            index_offset += fv;

            // per-face material
            shapes[s].mesh.material_ids[f];
        }
    }
}

SimpleObjFilter::~SimpleObjFilter() {

}

void SimpleObjFilter::draw3D() {

}
