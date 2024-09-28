#include "SkiaFilter.h"
#include "gpu/gl/GrGLInterface.h"
#include "gpu/ganesh/gl/GrGLDirectContext.h"
#include "gpu/ganesh/gl/GrGLBackendSurface.h"
#include "gpu/ganesh/SkSurfaceGanesh.h"
#include "gpu/GrBackendSurface.h"
#include "core/SkColorSpace.h"
#include "core/SkGraphics.h"
#include "core/SkBitmap.h"
#include "core/SkYUVAInfo.h"
#include "core/SkYUVAPixmaps.h"
#include "gpu/graphite/Image.h"

SkiaFilter::SkiaFilter(std::shared_ptr<AssetManager> &assetManager) : IFilter(assetManager, "") {
    SkGraphics::Init();
}

void SkiaFilter::drawTextures(VideoData *data) {

}

void SkiaFilter::setWindowSize(int width, int height) {
    rect.setXYWH(0, 0, width, height);
    paint.setColor(SK_ColorRED);
    IFilter::setWindowSize(width, height);
    auto backendInterface = GrGLMakeNativeInterface();
    skiaContext = GrDirectContexts::MakeGL(backendInterface);
    SkASSERT(skiaContext);
    GLint buffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &buffer);
    GLint stencil;
    glGetIntegerv(GL_STENCIL_BITS, &stencil);
    GLint samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    auto maxSamples = skiaContext->maxSurfaceSampleCountForColorType(kRGBA_8888_SkColorType);
    if (samples > maxSamples)
        samples = maxSamples;
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = buffer;
    fbInfo.fFormat = GL_RGBA8;
    auto _skRenderTarget = GrBackendRenderTargets::MakeGL(width, height, samples,
                                                          stencil, fbInfo);
    skiaSurface = SkSurfaces::WrapBackendRenderTarget(
            skiaContext.get(),
            _skRenderTarget,
            kBottomLeft_GrSurfaceOrigin,
            kRGBA_8888_SkColorType,
            nullptr,
            nullptr);
    SkASSERT(skiaSurface);
    skCanvas = skiaSurface->getCanvas();
}

void SkiaFilter::render(VideoData *data) {
    SkASSERT(skCanvas);
    skCanvas->clear(SK_ColorGREEN);
//    skCanvas->drawRect(rect, paint);
    auto info = SkYUVAInfo(SkISize::Make(data->videoWidth, data->videoHeight),
               SkYUVAInfo::PlaneConfig::kYUV,
               SkYUVAInfo::Subsampling::k420,
               SkYUVColorSpace::kJPEG_Full_SkYUVColorSpace);
//    auto rowBytes = new size_t *[3];
//    rowBytes[0] = data->y;
//    rowBytes[1] = data->u;
//    rowBytes[2] = data->v;
    auto pixelInfo = SkYUVAPixmapInfo(info, SkYUVAPixmapInfo::DataType::kUnorm8, nullptr);

    auto pixelMaps = SkYUVAPixmaps::Allocate(pixelInfo);
    auto skImage = SkImages::TextureFromYUVAPixmaps(nullptr, pixelMaps);
    skCanvas->drawImage(skImage, 0, 0);
    skiaContext->flush();
}
