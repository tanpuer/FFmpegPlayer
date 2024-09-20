#pragma once


#include "IFilter.h"

class NV21Filter : public IFilter {

public:

    explicit NV21Filter(std::shared_ptr<AssetManager> &assetManager);

    void drawTextures(VideoData *data) override;

};
