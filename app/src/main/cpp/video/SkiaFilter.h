#pragma once

#include "IFilter.h"
#include "gpu/GrDirectContext.h"
#include "core/SkCanvas.h"
#include "core/SkSurface.h"
#include "YUV420PFilter.h"
#include "effects/SkRuntimeEffect.h"
#include "core/SkFont.h"
#include "core/SkFontMgr.h"

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

    SkPaint paint;

    sk_sp<SkRuntimeEffect> runtimeEffect = nullptr;

    std::shared_ptr<AssetManager> assetManager = nullptr;

    sk_sp<SkFontMgr> fontMgr = nullptr;

    std::unique_ptr<SkFont> font = nullptr;

    SkPaint titlePaint;

    SkString familyName;

};
