#include "SimpleObjFilter.h"

SimpleObjFilter::SimpleObjFilter(std::shared_ptr<AssetManager> &assetManager, VideoYUVType type)
        : SkiaFilterWith3D(assetManager, type) {

}

SimpleObjFilter::~SimpleObjFilter() {

}

void SimpleObjFilter::draw3D() {

}
