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

class SkiaFilterWith3D : public IFilter {

public:

    explicit SkiaFilterWith3D(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type);

    void drawTextures(VideoData *data) override;

    virtual void setWindowSize(int width, int height) override;

    virtual void render(VideoData *data) override;

    void setTitle(const char *title) override;

    virtual void draw3D() = 0;

    virtual float getWidthRatio() { return 1.0f; }

protected:

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

protected:

    GLuint skiaFramebuffer = 0;
    GLuint skiaTexture = 0;
    GLint skiaTextureLocation = -1;
    int skiaWidth = 0;
    int skiaHeight = 0;

};
