#include "SkiaFilterWith3D.h"
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
#include "skparagraph/include/TypefaceFontProvider.h"
#include "skparagraph/include/FontCollection.h"

SkiaFilterWith3D::SkiaFilterWith3D(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type)
        : IFilter(
        assetManager, "") {
    SkGraphics::Init();
    this->assetManager = assetManager;
    this->type = type;

    fontMgr = SkFontMgr_New_Android(nullptr);
    auto fontProvider = sk_make_sp<TypefaceFontProvider>();
    auto fontData = assetManager->readImage("font/AlimamaFangYuanTiVF-Thin.ttf");
    auto data = SkData::MakeWithCopy(fontData->content, fontData->length);
    auto typeface = fontMgr->makeFromData(std::move(data));
    fontProvider->registerTypeface(typeface, SkString("Alimama"));
    delete fontData;
    fontCollection = sk_make_sp<FontCollection>();
    fontCollection->setAssetFontManager(std::move(fontProvider));
    fontCollection->enableFontFallback();

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

void SkiaFilterWith3D::drawTextures(VideoData *data) {

}

void SkiaFilterWith3D::setWindowSize(int width, int height) {
    skiaSize = std::min(width, height);
    if (skiaFramebuffer != 0) {
        glDeleteFramebuffers(1, &skiaFramebuffer);
        glDeleteTextures(1, &skiaTexture);
    }
    glGenFramebuffers(1, &skiaFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, skiaFramebuffer);
    glGenTextures(1, &skiaTexture);
    glBindTexture(GL_TEXTURE_2D, skiaTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, skiaSize, skiaSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skiaTexture, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        ALOGE("Framebuffer incomplete: %d", status)
    }
    glBindFramebuffer(GL_FRAMEBUFFER, skiaFramebuffer);

    paint.setAntiAlias(true);
    paint.setColor(SK_ColorRED);
    IFilter::setWindowSize(width, height);
    auto backendInterface = GrGLMakeNativeInterface();
    skiaContext = GrDirectContexts::MakeGL(backendInterface);
    SkASSERT(skiaContext);

    GrGLTextureInfo glInfo;
    glInfo.fID = skiaTexture;
    glInfo.fTarget = GL_TEXTURE_2D;
    glInfo.fFormat = GL_RGBA8;
    auto backendTexture = GrBackendTextures::MakeGL(skiaSize, skiaSize, skgpu::Mipmapped::kNo,
                                                    glInfo);
    SkASSERT(backendTexture.isValid());
    skiaSurface = SkSurfaces::WrapBackendTexture(
            skiaContext.get(),
            backendTexture,
            kBottomLeft_GrSurfaceOrigin,
            0,
            kRGBA_8888_SkColorType,
            nullptr,
            nullptr
    );
    skCanvas = skiaSurface->getCanvas();
}

void SkiaFilterWith3D::render(VideoData *data) {
    glBindFramebuffer(GL_FRAMEBUFFER, skiaFramebuffer);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        ALOGE("glCheckFramebufferStatus status:%d", status);
    }
    SkASSERT(skCanvas);
    skiaContext->resetContext();

//    skCanvas->drawColor(SkColorSetARGB(0xCC, 0xFF, 0x00, 0xFF), SkBlendMode::kSrcOut);
    skCanvas->drawColor(SkColorSetARGB(0xCC, 0xFF, 0x00, 0xFF), SkBlendMode::kDstATop);
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
    float widthRatio = skiaSize * 1.0f / data->videoWidth;
    float heightRatio = skiaSize * 1.0f / data->videoHeight;
    float ratio = std::min(widthRatio, heightRatio);
    builder.uniform("widthRatio") = ratio;
    builder.uniform("heightRatio") = ratio;
    sk_sp<SkShader> shader = builder.makeShader();
    paint.setShader(shader);
    skCanvas->save();

    if (widthRatio > heightRatio) {
        skCanvas->translate((skiaSize - data->videoWidth * ratio) / 2.0, 0);
    } else {
        skCanvas->translate(0, (skiaSize - data->videoHeight * ratio) / 2.0);
    }
    skCanvas->drawRect(SkRect::MakeXYWH(0, 0, data->videoWidth * ratio, data->videoHeight * ratio),
                       paint);
    skCanvas->restore();
    if (paragraph != nullptr) {
        paragraph->layout(skiaSize);
        paragraph->paint(skCanvas, 0, 0);
    }

    skiaContext->flushAndSubmit(GrSyncCpu::kNo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewWidth, viewHeight);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(true);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
//    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    glEnable(GL_DEPTH_WRITEMASK);
//    glEnable(GL_DEPTH);
//    glEnable(GL_DITHER);
//    glDepthRangef(0.0f, 1.0f);
//    glClearDepthf(1.0f);
//    glEnable(GL_POLYGON_OFFSET_FILL);
//    glEnable(GL_SCISSOR_TEST);

    draw3D();
}

void SkiaFilterWith3D::setTitle(const char *title) {
    IFilter::setTitle(title);
    skia::textlayout::ParagraphStyle paraStyle;
    auto paragraphBuilder = ParagraphBuilder::make(paraStyle, fontCollection);
    TextStyle textStyle;
    textStyle.setFontSize(100);
    textStyle.setColor(SK_ColorGREEN);
    textStyle.setFontFamilies({SkString("Alimama")});
    paragraphBuilder->pushStyle(textStyle);
    paragraphBuilder->addText(title);
    paragraph = paragraphBuilder->Build();
}
