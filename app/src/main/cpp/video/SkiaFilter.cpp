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
}

void SkiaFilter::drawTextures(VideoData *data) {

}

void SkiaFilter::setWindowSize(int width, int height) {
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

    static const char *kYUVtoRGBShader = R"(
        uniform shader y_tex;
        uniform shader u_tex;
        uniform shader v_tex;

        uniform float widthRatio;
        uniform float heightRatio;

        half4 main(float2 coord) {
            float2 texCoord = float2(coord.x / widthRatio, coord.y / heightRatio);
//            float2 texCoord = coord;
            float2 uv_coord = texCoord / 2.0; // Because U/V planes are half the size
            half y = y_tex.eval(texCoord).r;
            half u = u_tex.eval(uv_coord).r - 0.5;
            half v = v_tex.eval(uv_coord).r - 0.5;
            half r = y + 1.402 * v;
            half g = y - 0.344 * u - 0.714 * v;
            half b = y + 1.772 * u;
            return half4(r, g, b, 1.0);
        }
    )";
    auto [effect, error] = SkRuntimeEffect::MakeForShader(SkString(kYUVtoRGBShader));
    if (!effect) {
        ALOGD("set shader source failed %s", error.data())
        return;
    }
    runtimeEffect = effect;
}

void SkiaFilter::render(VideoData *data) {
    SkASSERT(skCanvas);

    skCanvas->clear(SK_ColorBLACK);

    auto width = data->videoWidth;
    auto height = data->height;
    auto y_imageInfo = SkImageInfo::Make(data->lineSizeY, height, SkColorType::kGray_8_SkColorType,
                                         kPremul_SkAlphaType);
    auto u_imageInfo = SkImageInfo::Make(data->lineSizeU, height / 2, SkColorType::kGray_8_SkColorType,
                                          kPremul_SkAlphaType);
    auto v_imageInfo = SkImageInfo::Make(data->lineSizeV, height / 2, SkColorType::kGray_8_SkColorType,
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
    float screen_ratio = viewWidth * 1.0f / viewHeight;
    float video_ratio = data->videoWidth * 1.0f / data->videoHeight;
    float ratio = 1.5;
    builder.uniform("widthRatio") = ratio;
    builder.uniform("heightRatio") = ratio;
    sk_sp<SkShader> shader = builder.makeShader();
    paint.setShader(shader);
    skCanvas->drawRect(SkRect::MakeXYWH(0, 0, data->videoWidth * ratio, data->videoHeight * ratio), paint);
    skiaContext->flush();
}
