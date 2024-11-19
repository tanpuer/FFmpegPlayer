#include "SimpleObjFilter.h"

#define TINYOBJLOADER_IMPLEMENTATION

#include "obj/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION

#include "obj/stb_image.h"

std::vector<tinyobj::material_t> materials;

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
    float v10[3];
    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];

    float v20[3];
    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];

    N[0] = v10[1] * v20[2] - v10[2] * v20[1];
    N[1] = v10[2] * v20[0] - v10[0] * v20[2];
    N[2] = v10[0] * v20[1] - v10[1] * v20[0];

    float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
        float len = sqrtf(len2);

        N[0] /= len;
        N[1] /= len;
        N[2] /= len;
    }
}

static void normalizeVector(vec3 &v) {
    float len2 = v.v[0] * v.v[0] + v.v[1] * v.v[1] + v.v[2] * v.v[2];
    if (len2 > 0.0f) {
        float len = sqrtf(len2);

        v.v[0] /= len;
        v.v[1] /= len;
        v.v[2] /= len;
    }
}

static void computeSmoothingShape(tinyobj::attrib_t &inattrib, tinyobj::shape_t &inshape,
                                  std::vector<std::pair<unsigned int, unsigned int>> &sortedids,
                                  unsigned int idbegin, unsigned int idend,
                                  std::vector<tinyobj::shape_t> &outshapes,
                                  tinyobj::attrib_t &outattrib) {
    unsigned int sgroupid = sortedids[idbegin].first;
    bool hasmaterials = inshape.mesh.material_ids.size();
    // Make a new shape from the set of faces in the range [idbegin, idend).
    outshapes.emplace_back();
    tinyobj::shape_t &outshape = outshapes.back();
    outshape.name = inshape.name;
    // Skip lines and points.

    std::unordered_map<unsigned int, unsigned int> remap;
    for (unsigned int id = idbegin; id < idend; ++id) {
        unsigned int face = sortedids[id].second;

        outshape.mesh.num_face_vertices.push_back(3); // always triangles
        if (hasmaterials)
            outshape.mesh.material_ids.push_back(inshape.mesh.material_ids[face]);
        outshape.mesh.smoothing_group_ids.push_back(sgroupid);
        // Skip tags.

        for (unsigned int v = 0; v < 3; ++v) {
            tinyobj::index_t inidx = inshape.mesh.indices[3 * face + v], outidx;
            assert(inidx.vertex_index != -1);
            auto iter = remap.find(inidx.vertex_index);
            // Smooth group 0 disables smoothing so no shared vertices in that case.
            if (sgroupid && iter != remap.end()) {
                outidx.vertex_index = (*iter).second;
                outidx.normal_index = outidx.vertex_index;
                outidx.texcoord_index = (inidx.texcoord_index == -1) ? -1 : outidx.vertex_index;
            } else {
                assert(outattrib.vertices.size() % 3 == 0);
                unsigned int offset = static_cast<unsigned int>(outattrib.vertices.size() / 3);
                outidx.vertex_index = outidx.normal_index = offset;
                outidx.texcoord_index = (inidx.texcoord_index == -1) ? -1 : offset;
                outattrib.vertices.push_back(inattrib.vertices[3 * inidx.vertex_index]);
                outattrib.vertices.push_back(inattrib.vertices[3 * inidx.vertex_index + 1]);
                outattrib.vertices.push_back(inattrib.vertices[3 * inidx.vertex_index + 2]);
                outattrib.normals.push_back(0.0f);
                outattrib.normals.push_back(0.0f);
                outattrib.normals.push_back(0.0f);
                if (inidx.texcoord_index != -1) {
                    outattrib.texcoords.push_back(inattrib.texcoords[2 * inidx.texcoord_index]);
                    outattrib.texcoords.push_back(inattrib.texcoords[2 * inidx.texcoord_index + 1]);
                }
                remap[inidx.vertex_index] = offset;
            }
            outshape.mesh.indices.push_back(outidx);
        }
    }
}

static void computeSmoothingShapes(tinyobj::attrib_t &inattrib,
                                   std::vector<tinyobj::shape_t> &inshapes,
                                   std::vector<tinyobj::shape_t> &outshapes,
                                   tinyobj::attrib_t &outattrib) {
    for (size_t s = 0, slen = inshapes.size(); s < slen; ++s) {
        tinyobj::shape_t &inshape = inshapes[s];

        unsigned int numfaces = static_cast<unsigned int>(inshape.mesh.smoothing_group_ids.size());
        assert(numfaces);
        std::vector<std::pair<unsigned int, unsigned int>> sortedids(numfaces);
        for (unsigned int i = 0; i < numfaces; ++i)
            sortedids[i] = std::make_pair(inshape.mesh.smoothing_group_ids[i], i);
        sort(sortedids.begin(), sortedids.end());

        unsigned int activeid = sortedids[0].first;
        unsigned int id = activeid, idbegin = 0, idend = 0;
        // Faces are now bundled by smoothing group id, create shapes from these.
        while (idbegin < numfaces) {
            while (activeid == id && ++idend < numfaces)
                id = sortedids[idend].first;
            computeSmoothingShape(inattrib, inshape, sortedids, idbegin, idend,
                                  outshapes, outattrib);
            activeid = id;
            idbegin = idend;
        }
    }
}

static void computeAllSmoothingNormals(tinyobj::attrib_t &attrib,
                                       std::vector<tinyobj::shape_t> &shapes) {
    vec3 p[3];
    for (size_t s = 0, slen = shapes.size(); s < slen; ++s) {
        const tinyobj::shape_t &shape(shapes[s]);
        size_t facecount = shape.mesh.num_face_vertices.size();
        assert(shape.mesh.smoothing_group_ids.size());

        for (size_t f = 0, flen = facecount; f < flen; ++f) {
            for (unsigned int v = 0; v < 3; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[3 * f + v];
                assert(idx.vertex_index != -1);
                p[v].v[0] = attrib.vertices[3 * idx.vertex_index];
                p[v].v[1] = attrib.vertices[3 * idx.vertex_index + 1];
                p[v].v[2] = attrib.vertices[3 * idx.vertex_index + 2];
            }

            // cross(p[1] - p[0], p[2] - p[0])
            float nx = (p[1].v[1] - p[0].v[1]) * (p[2].v[2] - p[0].v[2]) -
                       (p[1].v[2] - p[0].v[2]) * (p[2].v[1] - p[0].v[1]);
            float ny = (p[1].v[2] - p[0].v[2]) * (p[2].v[0] - p[0].v[0]) -
                       (p[1].v[0] - p[0].v[0]) * (p[2].v[2] - p[0].v[2]);
            float nz = (p[1].v[0] - p[0].v[0]) * (p[2].v[1] - p[0].v[1]) -
                       (p[1].v[1] - p[0].v[1]) * (p[2].v[0] - p[0].v[0]);

            // Don't normalize here.
            for (unsigned int v = 0; v < 3; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[3 * f + v];
                attrib.normals[3 * idx.normal_index] += nx;
                attrib.normals[3 * idx.normal_index + 1] += ny;
                attrib.normals[3 * idx.normal_index + 2] += nz;
            }
        }
    }

    assert(attrib.normals.size() % 3 == 0);
    for (size_t i = 0, nlen = attrib.normals.size() / 3; i < nlen; ++i) {
        tinyobj::real_t &nx = attrib.normals[3 * i];
        tinyobj::real_t &ny = attrib.normals[3 * i + 1];
        tinyobj::real_t &nz = attrib.normals[3 * i + 2];
        tinyobj::real_t len = sqrtf(nx * nx + ny * ny + nz * nz);
        tinyobj::real_t scale = len == 0 ? 0 : 1 / len;
        nx *= scale;
        ny *= scale;
        nz *= scale;
    }
}

static void computeSmoothingNormals(const tinyobj::attrib_t &attrib,
                                    const tinyobj::shape_t &shape,
                                    std::map<int, vec3> &smoothVertexNormals) {
    smoothVertexNormals.clear();
    std::map<int, vec3>::iterator iter;

    for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
        // Get the three indexes of the face (all faces are triangular)
        tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

        // Get the three vertex indexes and coordinates
        int vi[3];      // indexes
        float v[3][3];  // coordinates

        for (int k = 0; k < 3; k++) {
            vi[0] = idx0.vertex_index;
            vi[1] = idx1.vertex_index;
            vi[2] = idx2.vertex_index;
            assert(vi[0] >= 0);
            assert(vi[1] >= 0);
            assert(vi[2] >= 0);

            v[0][k] = attrib.vertices[3 * vi[0] + k];
            v[1][k] = attrib.vertices[3 * vi[1] + k];
            v[2][k] = attrib.vertices[3 * vi[2] + k];
        }

        // Compute the normal of the face
        float normal[3];
        CalcNormal(normal, v[0], v[1], v[2]);

        // Add the normal to the three vertexes
        for (size_t i = 0; i < 3; ++i) {
            iter = smoothVertexNormals.find(vi[i]);
            if (iter != smoothVertexNormals.end()) {
                // add
                iter->second.v[0] += normal[0];
                iter->second.v[1] += normal[1];
                iter->second.v[2] += normal[2];
            } else {
                smoothVertexNormals[vi[i]].v[0] = normal[0];
                smoothVertexNormals[vi[i]].v[1] = normal[1];
                smoothVertexNormals[vi[i]].v[2] = normal[2];
            }
        }

    }  // f

    // Normalize the normals, that is, make them unit vectors
    for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end();
         iter++) {
        normalizeVector(iter->second);
    }
}

static bool hasSmoothingGroup(const tinyobj::shape_t &shape) {
    for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++) {
        if (shape.mesh.smoothing_group_ids[i] > 0) {
            return true;
        }
    }
    return false;
}

static void
LoadObjAndConvert(
        const char *objPath,
        const char *mtlPath,
        float bmin[3], float bmax[3], std::shared_ptr<AssetManager> &assetManager,
        std::vector<DrawObject> *drawObjects,
        std::vector<tinyobj::material_t> &materials,
        std::map<std::string, GLuint> &textures) {
    tinyobj::attrib_t inattrib;
    std::vector<tinyobj::shape_t> inshapes;

    tinyobj::ObjReader reader;

    auto objStr = assetManager->readImage(objPath);
    auto mtlStr = assetManager->readImage(mtlPath);
    if (!reader.ParseFromString(objStr->content, mtlStr->content)) {
        if (!reader.Error().empty()) {
            ALOGE("TinyObjReader: %s", reader.Error().c_str())
        }
        return;
    }

    inattrib = reader.GetAttrib();
    inshapes = reader.GetShapes();
    materials = reader.GetMaterials();

    printf("# of vertices  = %d\n", (int) (inattrib.vertices.size()) / 3);
    printf("# of normals   = %d\n", (int) (inattrib.normals.size()) / 3);
    printf("# of texcoords = %d\n", (int) (inattrib.texcoords.size()) / 2);
    printf("# of materials = %d\n", (int) materials.size());
    printf("# of shapes    = %d\n", (int) inshapes.size());

    // Append `1t` material
    materials.push_back(tinyobj::material_t());

    for (size_t i = 0; i < materials.size(); i++) {
        ALOGD("material[%d].diffuse_texname = %s\n", int(i),
              materials[i].diffuse_texname.c_str());
    }

    // Load diffuse textures
    {
        for (size_t m = 0; m < materials.size(); m++) {
            tinyobj::material_t *mp = &materials[m];

            if (mp->diffuse_texname.length() > 0) {
                // Only load the texture if it is not already loaded
                if (textures.find(mp->diffuse_texname) == textures.end()) {
                    GLuint texture_id;
                    int w, h;
                    int comp;

                    std::string texture_filename = mp->diffuse_texname;

                    auto imageData = assetManager->readImage(texture_filename.c_str());
                    unsigned char *image = stbi_load_from_memory(
                            (const stbi_uc *) imageData->content, imageData->length, &w, &h,
                            &comp, STBI_default);
                    if (!image) {
                        ALOGE("Unable to load texture: %s", texture_filename.c_str())
                        return;
                    }
                    glGenTextures(1, &texture_id);
                    glBindTexture(GL_TEXTURE_2D, texture_id);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    if (comp == 3) {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
                                     GL_UNSIGNED_BYTE, image);
                    } else if (comp == 4) {
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                                     GL_UNSIGNED_BYTE, image);
                    } else {
                        assert(0);
                    }
                    glBindTexture(GL_TEXTURE_2D, 0);
                    stbi_image_free(image);
                    textures.insert(std::make_pair(mp->diffuse_texname, texture_id));
                }
            }
        }
    }

    bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
    bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

    bool regen_all_normals = inattrib.normals.size() == 0;
    tinyobj::attrib_t outattrib;
    std::vector<tinyobj::shape_t> outshapes;
    if (regen_all_normals) {
        computeSmoothingShapes(inattrib, inshapes, outshapes, outattrib);
        computeAllSmoothingNormals(outattrib, outshapes);
    }

    std::vector<tinyobj::shape_t> &shapes = regen_all_normals ? outshapes : inshapes;
    tinyobj::attrib_t &attrib = regen_all_normals ? outattrib : inattrib;

    {
        for (size_t s = 0; s < shapes.size(); s++) {
            DrawObject o;
            std::vector<float> buffer;  // pos(3float), normal(3float), color(3float)

            // Check for smoothing group and compute smoothing normals
            std::map<int, vec3> smoothVertexNormals;
            if (!regen_all_normals && (hasSmoothingGroup(shapes[s]) > 0)) {
                ALOGD("Compute smoothingNormal for shape %d", s)
                computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
            }

            for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
                tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
                tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
                tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

                int current_material_id = shapes[s].mesh.material_ids[f];

                if ((current_material_id < 0) ||
                    (current_material_id >= static_cast<int>(materials.size()))) {
                    // Invaid material ID. Use default material.
                    current_material_id =
                            materials.size() -
                            1;  // Default material is added to the last item in `materials`.
                }
                // if (current_material_id >= materials.size()) {
                //    std::cerr << "Invalid material index: " << current_material_id <<
                //    std::endl;
                //}
                //
                float diffuse[3];
                for (size_t i = 0; i < 3; i++) {
                    diffuse[i] = materials[current_material_id].diffuse[i];
                }
                float tc[3][2];
                if (attrib.texcoords.size() > 0) {
                    if ((idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) ||
                        (idx2.texcoord_index < 0)) {
                        // face does not contain valid uv index.
                        tc[0][0] = 0.0f;
                        tc[0][1] = 0.0f;
                        tc[1][0] = 0.0f;
                        tc[1][1] = 0.0f;
                        tc[2][0] = 0.0f;
                        tc[2][1] = 0.0f;
                    } else {
                        assert(attrib.texcoords.size() >
                               size_t(2 * idx0.texcoord_index + 1));
                        assert(attrib.texcoords.size() >
                               size_t(2 * idx1.texcoord_index + 1));
                        assert(attrib.texcoords.size() >
                               size_t(2 * idx2.texcoord_index + 1));

                        // Flip Y coord.
                        tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
                        tc[0][1] = 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1];
                        tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
                        tc[1][1] = 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1];
                        tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
                        tc[2][1] = 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1];
                    }
                } else {
                    tc[0][0] = 0.0f;
                    tc[0][1] = 0.0f;
                    tc[1][0] = 0.0f;
                    tc[1][1] = 0.0f;
                    tc[2][0] = 0.0f;
                    tc[2][1] = 0.0f;
                }

                float v[3][3];
                for (int k = 0; k < 3; k++) {
                    int f0 = idx0.vertex_index;
                    int f1 = idx1.vertex_index;
                    int f2 = idx2.vertex_index;
                    assert(f0 >= 0);
                    assert(f1 >= 0);
                    assert(f2 >= 0);

                    v[0][k] = attrib.vertices[3 * f0 + k];
                    v[1][k] = attrib.vertices[3 * f1 + k];
                    v[2][k] = attrib.vertices[3 * f2 + k];
                    bmin[k] = std::min(v[0][k], bmin[k]);
                    bmin[k] = std::min(v[1][k], bmin[k]);
                    bmin[k] = std::min(v[2][k], bmin[k]);
                    bmax[k] = std::max(v[0][k], bmax[k]);
                    bmax[k] = std::max(v[1][k], bmax[k]);
                    bmax[k] = std::max(v[2][k], bmax[k]);
                }

                float n[3][3];
                {
                    bool invalid_normal_index = false;
                    if (attrib.normals.size() > 0) {
                        int nf0 = idx0.normal_index;
                        int nf1 = idx1.normal_index;
                        int nf2 = idx2.normal_index;

                        if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0)) {
                            // normal index is missing from this face.
                            invalid_normal_index = true;
                        } else {
                            for (int k = 0; k < 3; k++) {
                                assert(size_t(3 * nf0 + k) < attrib.normals.size());
                                assert(size_t(3 * nf1 + k) < attrib.normals.size());
                                assert(size_t(3 * nf2 + k) < attrib.normals.size());
                                n[0][k] = attrib.normals[3 * nf0 + k];
                                n[1][k] = attrib.normals[3 * nf1 + k];
                                n[2][k] = attrib.normals[3 * nf2 + k];
                            }
                        }
                    } else {
                        invalid_normal_index = true;
                    }

                    if (invalid_normal_index && !smoothVertexNormals.empty()) {
                        // Use smoothing normals
                        int f0 = idx0.vertex_index;
                        int f1 = idx1.vertex_index;
                        int f2 = idx2.vertex_index;

                        if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                            n[0][0] = smoothVertexNormals[f0].v[0];
                            n[0][1] = smoothVertexNormals[f0].v[1];
                            n[0][2] = smoothVertexNormals[f0].v[2];

                            n[1][0] = smoothVertexNormals[f1].v[0];
                            n[1][1] = smoothVertexNormals[f1].v[1];
                            n[1][2] = smoothVertexNormals[f1].v[2];

                            n[2][0] = smoothVertexNormals[f2].v[0];
                            n[2][1] = smoothVertexNormals[f2].v[1];
                            n[2][2] = smoothVertexNormals[f2].v[2];

                            invalid_normal_index = false;
                        }
                    }

                    if (invalid_normal_index) {
                        // compute geometric normal
                        CalcNormal(n[0], v[0], v[1], v[2]);
                        n[1][0] = n[0][0];
                        n[1][1] = n[0][1];
                        n[1][2] = n[0][2];
                        n[2][0] = n[0][0];
                        n[2][1] = n[0][1];
                        n[2][2] = n[0][2];
                    }
                }

                for (int k = 0; k < 3; k++) {
                    buffer.push_back(v[k][0]);
                    buffer.push_back(v[k][1]);
                    buffer.push_back(v[k][2]);
                    buffer.push_back(n[k][0]);
                    buffer.push_back(n[k][1]);
                    buffer.push_back(n[k][2]);
                    // Combine normal and diffuse to get color.
                    float normal_factor = 0.2;
                    float diffuse_factor = 1 - normal_factor;
                    float c[3] = {n[k][0] * normal_factor + diffuse[0] * diffuse_factor,
                                  n[k][1] * normal_factor + diffuse[1] * diffuse_factor,
                                  n[k][2] * normal_factor + diffuse[2] * diffuse_factor};
                    float len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
                    if (len2 > 0.0f) {
                        float len = sqrtf(len2);

                        c[0] /= len;
                        c[1] /= len;
                        c[2] /= len;
                    }
                    buffer.push_back(c[0] * 0.5 + 0.5);
                    buffer.push_back(c[1] * 0.5 + 0.5);
                    buffer.push_back(c[2] * 0.5 + 0.5);

                    buffer.push_back(tc[k][0]);
                    buffer.push_back(tc[k][1]);
                }
            }

            o.vb_id = 0;
            o.numTriangles = 0;

            // OpenGL viewer does not support texturing with per-face material.
            if (shapes[s].mesh.material_ids.size() > 0 &&
                shapes[s].mesh.material_ids.size() > s) {
                o.material_id = shapes[s].mesh.material_ids[0];  // use the material ID
                // of the first face.
            } else {
                o.material_id = materials.size() - 1;  // = ID for default material.
            }
            printf("shape[%d] material_id %d\n", int(s), int(o.material_id));

            if (buffer.size() > 0) {
                glGenBuffers(1, &o.vb_id);
                glBindBuffer(GL_ARRAY_BUFFER, o.vb_id);
                glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
                             &buffer.at(0), GL_STATIC_DRAW);
                o.numTriangles = buffer.size() / (3 + 3 + 3 + 2) /
                                 3;  // 3:vtx, 3:normal, 3:col, 2:texcoord

                ALOGD("shape[%d] # of triangles = %d\n", static_cast<int>(s), o.numTriangles)
            }

            drawObjects->push_back(o);
        }
    }

    ALOGD("bmin = %f, %f, %f\n", bmin[0], bmin[1], bmin[2])
    ALOGD("bmax = %f, %f, %f\n", bmax[0], bmax[1], bmax[2])
}

SimpleObjFilter::SimpleObjFilter(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type)
        : SkiaFilterWith3D(assetManager, type) {

    auto vertex_shader_string = assetManager->readFile("obj_vertex_shader.glsl");
    auto fragment_shader_string = assetManager->readFile("obj_fragment_shader.glsl");
    auto vertexShader = loadShader(GL_VERTEX_SHADER, vertex_shader_string);
    auto fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragment_shader_string);
    program = createShaderProgram(vertexShader, fragmentShader);
}

SimpleObjFilter::~SimpleObjFilter() {

}

void SimpleObjFilter::draw3D() {
    if (gDrawObjects.empty()) {
        float bmin[3], bmax[3];
        LoadObjAndConvert("tv/tv-2.obj", "tv/tv-2.mtl", bmin, bmax, assetManager, &gDrawObjects, materials, textures);

        float maxExtent = 0.5f * (bmax[0] - bmin[0]);
        if (maxExtent < 0.5f * (bmax[1] - bmin[1])) {
            maxExtent = 0.5f * (bmax[1] - bmin[1]);
        }
        if (maxExtent < 0.5f * (bmax[2] - bmin[2])) {
            maxExtent = 0.5f * (bmax[2] - bmin[2]);
        }
    }

    checkGLError("draw3D");
    glUseProgram(program);
    auto iViewMatrix = glGetUniformLocation(program, "iViewMatrix");

    ESMatrix viewMatrix;
    setLookAtM(&viewMatrix, 0, 0.0f, 0.0f, 3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    ESMatrix modelMatrix;
    setIdentityM(&modelMatrix);
//    static float rotate = 0.0f;
//    rotate += 1.0f;
//    rotateM(&modelMatrix, rotate, 0.0f, 1.0f, 0.0f);
    ESMatrix projectMatrix;
    perspectiveM(&projectMatrix, 0, 45.0f, (float) viewWidth / (float) viewHeight, 0.1f, 100.0f);
    ESMatrix mvMatrix;
    multiplyMM(&mvMatrix, &viewMatrix, &modelMatrix);
    ESMatrix mvpMatrix;
    multiplyMM(&mvpMatrix, &projectMatrix, &mvMatrix);

    glUniformMatrix4fv(iViewMatrix, 1, GL_FALSE, mvpMatrix.m);
    checkGLError("HYPlayer::setUniforms");

    GLsizei stride = (3 + 3 + 3 + 2) * sizeof(float);
    for (size_t i = 0; i < gDrawObjects.size(); i++) {
        DrawObject o = gDrawObjects[i];
        if (o.vb_id < 1) {
            continue;
        }

        glBindBuffer(GL_ARRAY_BUFFER, o.vb_id);
        glBindTexture(GL_TEXTURE_2D, 0);
        if ((o.material_id < materials.size())) {
            std::string diffuse_texname = materials[o.material_id].diffuse_texname;
            if (textures.find(diffuse_texname) != textures.end()) {
                skiaTextureLocation = glGetUniformLocation(program, "skia_texture");
                glActiveTexture(GL_TEXTURE0 + textures[diffuse_texname]);
                glBindTexture(GL_TEXTURE_2D, textures[diffuse_texname]);
                glUniform1i(skiaTextureLocation, textures[diffuse_texname]);
            } else if (o.numTriangles > 5000) {
                //TODO!!!!!
                skiaTextureLocation = glGetUniformLocation(program, "skia_texture");
                glActiveTexture(GL_TEXTURE0 + skiaTexture);
                glBindTexture(GL_TEXTURE_2D, skiaTexture);
                glUniform1i(skiaTextureLocation, skiaTexture);
            }

        }
        // Vertex position
        GLint posAttrib = glGetAttribLocation(program, "position");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, stride, (const void *) 0);

        GLint normalAttrib = glGetAttribLocation(program, "normal");
        if (normalAttrib > 0) {
            glEnableVertexAttribArray(normalAttrib);
            glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, stride,
                                  (const void *) (sizeof(float) * 3));
        }

        GLint colorAttrib = glGetAttribLocation(program, "color");
        if (colorAttrib > 0) {
            glEnableVertexAttribArray(colorAttrib);
            glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, stride,
                                  (const void *) (sizeof(float) * 6));
        }

        GLint texCoordAttrib = glGetAttribLocation(program, "texCoord");
        glEnableVertexAttribArray(texCoordAttrib);
        glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, stride,
                              (const void *) (sizeof(float) * 9));

        glDrawArrays(GL_TRIANGLES, 0, 3 * o.numTriangles);
        checkGLError("drawarrays");
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
