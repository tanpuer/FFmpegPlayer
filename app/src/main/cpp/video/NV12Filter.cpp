#include "NV12Filter.h"

NV12Filter::NV12Filter(std::shared_ptr<AssetManager> &assetManager)
        : IFilter(assetManager, "nv12_fragment_shader.glsl") {

}

void NV12Filter::drawTextures(VideoData *data) {
    uTextureYLocation = glGetUniformLocation(program, "uTextureY");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, yTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, data->lineSizeY, data->height, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, data->y);
    glUniform1i(uTextureYLocation, 0);

    uTextureULocation = glGetUniformLocation(program, "uTextureU");
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, data->lineSizeU, data->height / 2, 0,
                 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data->u);
    glUniform1i(uTextureULocation, 1);
}
