#include "Simple3DFilter.h"

Simple3DFilter::Simple3DFilter(std::shared_ptr<AssetManager> &assetManager,
                               VideoYUVType type) : SkiaFilterWith3D(assetManager, type) {
    auto vertex_shader_string = assetManager->readFile("video_vertex_shader.glsl");
    auto fragment_shader_string = assetManager->readFile("simple_3d_fragment_shader.glsl");
    auto vertexShader = loadShader(GL_VERTEX_SHADER, vertex_shader_string);
    auto fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragment_shader_string);
    program = createShaderProgram(vertexShader, fragmentShader);
}

Simple3DFilter::~Simple3DFilter() {

}

void Simple3DFilter::draw3D() {
    glUseProgram(program);
    auto iViewMatrix = glGetUniformLocation(program, "iViewMatrix");

    ESMatrix viewMatrix;
    setLookAtM(&viewMatrix, 0, -5.0f, -5.0f, -3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    ESMatrix modelMatrix;
    setIdentityM(&modelMatrix);
    static float rotate = 0.0f;
    rotate += 1.0f;
    rotateM(&modelMatrix, rotate, 0.0f, 1.0f, 0.0f);
    ESMatrix projectMatrix;
    perspectiveM(&projectMatrix, 0, 45.0f, (float) viewWidth / (float) viewHeight, 0.1f, 100.0f);
    ESMatrix mvMatrix;
    multiplyMM(&mvMatrix, &viewMatrix, &modelMatrix);
    ESMatrix mvpMatrix;
    multiplyMM(&mvpMatrix, &projectMatrix, &mvMatrix);

    glUniformMatrix4fv(iViewMatrix, 1, GL_FALSE, mvpMatrix.m);
    checkGLError("HYPlayer::setUniforms");
    aPositionLocation = glGetAttribLocation(program, "aPosition");
    glEnableVertexAttribArray(aPositionLocation);
    glVertexAttribPointer(aPositionLocation, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vertices);
    aTextureCoordinateLocation = glGetAttribLocation(program, "aTextureCoord");
    glEnableVertexAttribArray(aTextureCoordinateLocation);
    glVertexAttribPointer(aTextureCoordinateLocation, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
                          &vertices[3]);
    checkGLError("HYPlayer::setAttributes");

    skiaTextureLocation = glGetUniformLocation(program, "skia_texture");
    checkGLError("HYPlayer::glGetUniformLocation");
    glActiveTexture(GL_TEXTURE0 + skiaTexture);
    glBindTexture(GL_TEXTURE_2D, skiaTexture);
    glUniform1i(skiaTextureLocation, skiaTexture);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indices);
    checkGLError("HYPlayer::glDrawArrays");
    glDisableVertexAttribArray(aPositionLocation);
    glDisableVertexAttribArray(aTextureCoordinateLocation);
    glBindTexture(GL_TEXTURE_2D, 0);
}
