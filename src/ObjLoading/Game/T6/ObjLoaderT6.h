#pragma once

#include <unordered_map>
#include <memory>

#include "IObjLoader.h"
#include "AssetLoading/IAssetLoader.h"
#include "SearchPath/ISearchPath.h"
#include "Game/T6/T6.h"

namespace T6
{
    class ObjLoader final : public IObjLoader
    {
        static const int IPAK_READ_HASH;
        static const int GLOBAL_HASH;

        std::unordered_map<asset_type_t, std::unique_ptr<IAssetLoader>> m_asset_loaders_by_type;

        static void LoadIPakForZone(ISearchPath* searchPath, const std::string& ipakName, Zone* zone);

        static void LoadImageFromIwi(GfxImage* image, ISearchPath* searchPath, Zone* zone);
        static void LoadImageFromLoadDef(GfxImage* image, Zone* zone);
        static void LoadImageData(ISearchPath* searchPath, Zone* zone);

        static bool IsMpZone(Zone* zone);
        static bool IsZmZone(Zone* zone);
        static void LoadCommonIPaks(ISearchPath* searchPath, Zone* zone);

    public:
        ObjLoader();

        bool SupportsZone(Zone* zone) const override;

        void LoadReferencedContainersForZone(ISearchPath* searchPath, Zone* zone) const override;
        void UnloadContainersOfZone(Zone* zone) const override;

        void LoadObjDataForZone(ISearchPath* searchPath, Zone* zone) const override;

        bool LoadAssetForZone(AssetLoadingContext* context, asset_type_t assetType, const std::string& assetName) const override;
    };
}
