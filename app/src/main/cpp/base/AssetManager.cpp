#include <bits/seek_constants.h>
#include "AssetManager.h"
#include "native_log.h"

AssetManager::AssetManager(JNIEnv *env, jobject javaAssetManager) {
    asset_manager = AAssetManager_fromJava(env, javaAssetManager);
}

void AssetManager::openVideo(const char *path) {
    asset = AAssetManager_open(asset_manager, path, AASSET_MODE_STREAMING);
}

int AssetManager::read(uint8_t *buffer, int length) {
    return AAsset_read(asset, buffer, length);
}

off_t AssetManager::seek(off_t offset, int whence) {
    return AAsset_seek(asset, offset, whence);
}

void AssetManager::closeVideo() {
    AAsset_close(asset);
    asset = nullptr;
}

char *AssetManager::readFile(const char *path) {
    AAsset *asset = AAssetManager_open(asset_manager, path, AASSET_MODE_BUFFER);
    off_t fileLength = AAsset_getLength(asset);
    char *fileContent = new char[fileLength + 1];
    AAsset_read(asset, fileContent, fileLength);
    fileContent[fileLength] = '\0';
    AAsset_close(asset);
    return fileContent;
}
