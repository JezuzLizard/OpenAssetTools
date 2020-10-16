#pragma once

#include "Loading/AssetLoadingActions.h"
#include "Game/T6/T6.h"

namespace T6
{
    class Actions_GfxImage final : public AssetLoadingActions
    {
    public:
        explicit Actions_GfxImage(Zone* zone);

        void OnImageLoaded(GfxImage* image) const;
        void LoadImageData(GfxImageLoadDef* loadDef, GfxImage* image) const;
    };
}
