#pragma once

#include "IFilter.h"

class YUV420PFilter : public IFilter {

public:

    explicit YUV420PFilter(std::shared_ptr<AssetManager> &assetManager);

    void drawTextures(VideoData *data) override;
};
