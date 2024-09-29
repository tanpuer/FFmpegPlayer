#pragma once

#include "IFilter.h"
#include "gpu/GrDirectContext.h"
#include "core/SkCanvas.h"
#include "core/SkSurface.h"
#include "YUV420PFilter.h"

class SkiaFilter : public IFilter {

public:

    explicit SkiaFilter(std::shared_ptr<AssetManager> &assetManager);

    void drawTextures(VideoData *data) override;

    virtual void setWindowSize(int width, int height) override;

    virtual void render(VideoData *data) override;

private:

    sk_sp<SkSurface> skiaSurface = nullptr;

    sk_sp<GrDirectContext> skiaContext = nullptr;

    SkCanvas *skCanvas = nullptr;

    SkRect rect;

    SkPaint paint;

private:

    GLuint videoFrameBuffer = 0;
    GLuint videoTexture = 0;

    std::unique_ptr<YUV420PFilter> yuv420PFilter = nullptr;

};
