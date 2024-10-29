#pragma once

#include "VideoData.h"
#include "AssetManager.h"
#include "memory"
#include "GLES3/gl3.h"
#include "matrix_util.h"
#include "AssetManager.h"
#include "gl_utils.h"
#include "string"

static GLfloat videoVertex[] = {
        1.0f, 1.0f,
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
};

static GLfloat videoTexture[] = {
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};

class IFilter {

public:

    IFilter(std::shared_ptr<AssetManager> &assetManager, const char *fragmentShaderPath) {
        if (strlen(fragmentShaderPath) == 0) {
            return;
        }
        auto vertex_shader_string = assetManager->readFile("video_vertex_shader.glsl");
        auto fragment_shader_string = assetManager->readFile(fragmentShaderPath);
        auto vertexShader = loadShader(GL_VERTEX_SHADER, vertex_shader_string);
        auto fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragment_shader_string);
        program = createShaderProgram(vertexShader, fragmentShader);
        yTexture = createTexture(GL_TEXTURE_2D);
        uTexture = createTexture(GL_TEXTURE_2D);
        vTexture = createTexture(GL_TEXTURE_2D);
    };

    virtual void setWindowSize(int width, int height) {
        if (viewWidth != width || viewHeight != height) {
            viewSizeChanged = true;
            viewWidth = width;
            viewHeight = height;
        }
    }

    virtual ~IFilter() {
        glDeleteTextures(1, &yTexture);
        glDeleteTextures(1, &uTexture);
        glDeleteTextures(1, &vTexture);
        glDeleteProgram(program);
    };

    virtual void render(VideoData *data) {
        checkVideoSize(data);

        glUseProgram(program);
        auto iViewMatrix = glGetUniformLocation(program, "iViewMatrix");
        glUniformMatrix4fv(iViewMatrix, 1, GL_FALSE, matrix.m);
        checkGLError("HYPlayer::setUniforms");
        aPositionLocation = glGetAttribLocation(program, "aPosition");
        glEnableVertexAttribArray(aPositionLocation);
        glVertexAttribPointer(aPositionLocation, 2, GL_FLOAT, GL_FALSE, 8, videoVertex);
        aTextureCoordinateLocation = glGetAttribLocation(program, "aTextureCoord");
        glEnableVertexAttribArray(aTextureCoordinateLocation);
        glVertexAttribPointer(aTextureCoordinateLocation, 2, GL_FLOAT, GL_FALSE, 8, videoTexture);
        checkGLError("HYPlayer::setAttributes");

        drawTextures(data);

        GLint vertexCount = sizeof(videoVertex) / (sizeof(videoVertex[0]) * 2);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glDisableVertexAttribArray(aPositionLocation);
        glDisableVertexAttribArray(aTextureCoordinateLocation);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    virtual void drawTextures(VideoData *data) = 0;

    virtual void setTitle(const char* title) {
        this->title = title;
    }

private:

    void checkVideoSize(VideoData *data) {
        if (data->videoWidth != videoWidth || data->videoHeight != videoHeight || viewSizeChanged) {
            videoWidth = data->videoWidth;
            videoHeight = data->videoHeight;
            viewSizeChanged = false;
            setIdentityM(&matrix);
            rotateM(&matrix, 180, 0, 0, 1);
            rotateM(&matrix, 180, 0, 1, 0);
            float screen_ratio = viewWidth * 1.0f / viewHeight;
            float video_ratio = data->videoWidth * 1.0f / data->videoHeight;
            if (video_ratio > screen_ratio) {
                scaleM(&matrix, 0, 1.0f, screen_ratio / video_ratio, 1.0f);
            } else {
                scaleM(&matrix, 0, video_ratio / screen_ratio, 1.0f, 1.0f);
            }
        }
    }

protected:

    GLuint yTexture = 0, uTexture = 0, vTexture = 0;

    GLuint program = 0;

    ESMatrix matrix = ESMatrix();

    GLuint aPositionLocation = 0, aTextureCoordinateLocation = 0;

    GLint uTextureYLocation = -1.0, uTextureULocation = -1.0, uTextureVLocation = -1.0;

    int viewWidth = 0, viewHeight = 0;

    int videoWidth = 0, videoHeight = 0;

    bool viewSizeChanged = true;

    std::string title;

};