#include "YUV420PFilter.h"
#include "gl_utils.h"

YUV420PFilter::YUV420PFilter(std::shared_ptr<AssetManager> &assetManager)
        : IFilter(assetManager, "yuv_420p_fragment_shader.glsl") {
}

void YUV420PFilter::drawTextures(VideoData *data) {
    uTextureYLocation = glGetUniformLocation(program, "uTextureY");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, data->lineSizeY, data->height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, data->y);
    glUniform1i(uTextureYLocation, 0);

    uTextureULocation = glGetUniformLocation(program, "uTextureU");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, data->lineSizeU, data->height / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, data->u);
    glUniform1i(uTextureULocation, 1);

    uTextureVLocation = glGetUniformLocation(program, "uTextureV");
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, vTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, data->lineSizeV, data->height / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, data->v);
    glUniform1i(uTextureVLocation, 2);
}
