#pragma once

#include "AssetLoading/BasicAssetLoader.h"
#include "Game/IW4/IW4.h"
#include "SearchPath/ISearchPath.h"

namespace IW4
{
    class AssetLoaderGameWorldSp final : public BasicAssetLoader<ASSET_TYPE_GAMEWORLD_SP, GameWorldSp>
    {
    public:
        _NODISCARD void* CreateEmptyAsset(const std::string& assetName, MemoryManager* memory) override;
    };
} // namespace IW4
