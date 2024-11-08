#pragma once

#include "IFilter.h"
#include "gpu/GrDirectContext.h"
#include "core/SkCanvas.h"
#include "core/SkSurface.h"
#include "YUV420PFilter.h"
#include "effects/SkRuntimeEffect.h"
#include "core/SkFont.h"
#include "core/SkFontMgr.h"
#include "skparagraph/include/TypefaceFontProvider.h"
#include "skparagraph/include/ParagraphBuilder.h"

using namespace skia::textlayout;

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

class SkiaFilterWith3D : public IFilter {

public:

    explicit SkiaFilterWith3D(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type);

    void drawTextures(VideoData *data) override;

    virtual void setWindowSize(int width, int height) override;

    virtual void render(VideoData *data) override;

    void setTitle(const char *title) override;

private:

    sk_sp<SkSurface> skiaSurface = nullptr;

    sk_sp<GrDirectContext> skiaContext = nullptr;

    SkCanvas *skCanvas = nullptr;

    SkPaint paint;

    sk_sp<SkRuntimeEffect> runtimeEffect = nullptr;

    std::shared_ptr<AssetManager> assetManager = nullptr;

    sk_sp<SkFontMgr> fontMgr = nullptr;

    sk_sp<FontCollection> fontCollection = nullptr;

    std::unique_ptr<Paragraph> paragraph = nullptr;

    SkPaint titlePaint;

    SkString familyName;

    VideoYUVType type = VideoYUVType::YUV420P;

private:

    GLuint skiaFramebuffer = 0;
    GLuint skiaTexture = 0;
    GLint skiaTextureLocation = -1;
    int skiaSize = 0;

};
