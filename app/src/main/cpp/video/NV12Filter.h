#pragma once

#include "IFilter.h"

class NV12Filter : public IFilter {

public:

    explicit NV12Filter(std::shared_ptr<AssetManager> &assetManager);

    void drawTextures(VideoData *data) override;


};
