#pragma once

#include <android/asset_manager_jni.h>

struct ImageData {
    const unsigned char *content;
    off_t length;
};

class AssetManager {

public:

    AssetManager(JNIEnv *env, jobject javaAssetManager);

    void openVideo(const char *path);

    int read(uint8_t *buffer, int length);

    off_t seek(off_t offset, int whence);

    void closeVideo();

    char *readFile(const char *path);

    ImageData *readImage(const char *path);

private:

    AAssetManager *asset_manager = nullptr;

    AAsset *asset = nullptr;

};
