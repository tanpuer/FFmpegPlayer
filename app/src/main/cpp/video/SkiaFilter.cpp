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
#include "ports/SkFontMgr_android.h"

SkiaFilter::SkiaFilter(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type) : IFilter(
        assetManager, "") {
    SkGraphics::Init();
    this->assetManager = assetManager;
    this->type = type;
    fontMgr = SkFontMgr_New_Android(nullptr);
    fontMgr->getFamilyName(0, &familyName);
    font = std::make_unique<SkFont>(
            fontMgr->legacyMakeTypeface(familyName.c_str(), SkFontStyle::Normal()));

    std::string path;
    if (type == VideoYUVType::YUV420P) {
        path = "skia_yuv420p_fragment_shader.glsl";
    } else if (type == VideoYUVType::NV12) {
        path = "skia_nv12_fragment_shader.glsl";
    } else if (type == VideoYUVType::NV21) {
        path = "skia_nv21_fragment_shader.glsl";
    }
    const char *kYUVtoRGBShader = assetManager->readFile("skia_yuv420p_fragment_shader.glsl");
    auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(kYUVtoRGBShader));
    if (!effect) {
        ALOGD("set shader source failed %s", error.data())
        return;
    }
    runtimeEffect = effect;
}

void SkiaFilter::drawTextures(VideoData *data) {

}

void SkiaFilter::setWindowSize(int width, int height) {
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
    skCanvas->clear(SK_ColorWHITE);
    auto width = data->videoWidth;
    auto height = data->height;
    auto y_imageInfo = SkImageInfo::Make(data->lineSizeY, height, SkColorType::kGray_8_SkColorType,
                                         kPremul_SkAlphaType);
    auto uvColorType = type == VideoYUVType::YUV420P
                       ? SkColorType::kGray_8_SkColorType
                       : SkColorType::kR8G8_unorm_SkColorType;
    auto u_imageInfo = SkImageInfo::Make(data->lineSizeU, height / 2, uvColorType,
                                         kPremul_SkAlphaType);
    auto v_imageInfo = SkImageInfo::Make(data->lineSizeV, height / 2, uvColorType,
                                         kPremul_SkAlphaType);
    sk_sp<SkData> y_data = SkData::MakeWithoutCopy(data->y, data->lineSizeY * height);
    sk_sp<SkData> u_data = SkData::MakeWithoutCopy(data->u, data->lineSizeU * (height / 2));
    sk_sp<SkData> v_data = SkData::MakeWithoutCopy(data->v, data->lineSizeV * (height / 2));
    auto y_image = SkImages::RasterFromData(y_imageInfo, y_data, data->lineSizeY);
    auto u_image = SkImages::RasterFromData(u_imageInfo, u_data, data->lineSizeU);
    auto v_image = SkImages::RasterFromData(v_imageInfo, v_data, data->lineSizeV);
    SkRuntimeShaderBuilder builder(runtimeEffect);
    builder.child("y_tex") = y_image->makeShader(SkSamplingOptions());
    builder.child("u_tex") = u_image->makeShader(SkSamplingOptions());
    builder.child("v_tex") = v_image->makeShader(SkSamplingOptions());
    float widthRatio = viewWidth * 1.0f / data->videoWidth;
    float heightRatio = viewHeight * 1.0f / data->videoHeight;
    float ratio = std::min(widthRatio, heightRatio);
    builder.uniform("widthRatio") = ratio;
    builder.uniform("heightRatio") = ratio;
    sk_sp<SkShader> shader = builder.makeShader();
    paint.setShader(shader);
    skCanvas->save();
    if (widthRatio > heightRatio) {
        skCanvas->translate((viewWidth - data->videoWidth * ratio) / 2.0, 0);
    } else {
        skCanvas->translate(0, (viewHeight - data->videoHeight * ratio) / 2.0);
    }
    skCanvas->drawRect(SkRect::MakeXYWH(0, 0, data->videoWidth * ratio, data->videoHeight * ratio),
                       paint);
    skCanvas->restore();
    font->setSize(100);
    skCanvas->drawSimpleText("Skia Draw", 9, SkTextEncoding::kUTF8, 0.0f, 100.0f, *font,
                             titlePaint);
    skiaContext->flush();
}
