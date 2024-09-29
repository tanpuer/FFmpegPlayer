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
#include "core/SkPictureRecorder.h"
#include "gpu/ganesh/SkImageGanesh.h"
#include "gpu/ganesh/gl/GrGLDefines.h"

SkiaFilter::SkiaFilter(std::shared_ptr<AssetManager> &assetManager) : IFilter(assetManager, "") {
    SkGraphics::Init();
    yuv420PFilter = std::make_unique<YUV420PFilter>(assetManager);
}

void SkiaFilter::drawTextures(VideoData *data) {

}

void SkiaFilter::setWindowSize(int width, int height) {
    yuv420PFilter->setWindowSize(width, height);
    glDeleteTextures(1, &videoTexture);
    glDeleteFramebuffers(1, &videoFrameBuffer);
    createFrameBuffer(&videoFrameBuffer, &videoTexture, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, videoFrameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, videoTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        ALOGE("Framebuffer is not complete!")
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    rect.setXYWH(0, 0, width, height);
    paint.setAntiAlias(true);
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
    fbInfo.fFBOID = 0;
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

    glBindFramebuffer(GL_FRAMEBUFFER, videoFrameBuffer);
    yuv420PFilter->render(data);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    skCanvas->clear(SK_ColorBLACK);

    GrGLTextureInfo textureInfo;
    textureInfo.fID = videoTexture;
    textureInfo.fTarget = GR_GL_TEXTURE_2D;
    textureInfo.fFormat = static_cast<GrGLenum>(GrTextureType::k2D);
    auto backendTexture = GrBackendTextures::MakeGL(videoWidth, videoHeight, skgpu::Mipmapped::kNo,
                                                    textureInfo);
    sk_sp<SkImage> image = SkImages::BorrowTextureFrom(
            skiaContext.get(),
            backendTexture, kTopLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType,
            kOpaque_SkAlphaType, nullptr, nullptr, nullptr);
    skCanvas->drawImage(image, 0, 0);
    skiaContext->flush();
}
