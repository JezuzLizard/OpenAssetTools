#include "ObjLoaderIW5.h"

#include "AssetLoaders/AssetLoaderAddonMapEnts.h"
#include "AssetLoaders/AssetLoaderClipMap.h"
#include "AssetLoaders/AssetLoaderComWorld.h"
#include "AssetLoaders/AssetLoaderFont.h"
#include "AssetLoaders/AssetLoaderFx.h"
#include "AssetLoaders/AssetLoaderFxImpactTable.h"
#include "AssetLoaders/AssetLoaderFxWorld.h"
#include "AssetLoaders/AssetLoaderGfxImage.h"
#include "AssetLoaders/AssetLoaderGfxLightDef.h"
#include "AssetLoaders/AssetLoaderGfxWorld.h"
#include "AssetLoaders/AssetLoaderGlassWorld.h"
#include "AssetLoaders/AssetLoaderLeaderboard.h"
#include "AssetLoaders/AssetLoaderLoadedSound.h"
#include "AssetLoaders/AssetLoaderLocalizeEntry.h"
#include "AssetLoaders/AssetLoaderMapEnts.h"
#include "AssetLoaders/AssetLoaderMaterial.h"
#include "AssetLoaders/AssetLoaderMenuDef.h"
#include "AssetLoaders/AssetLoaderMenuList.h"
#include "AssetLoaders/AssetLoaderPathData.h"
#include "AssetLoaders/AssetLoaderPhysCollmap.h"
#include "AssetLoaders/AssetLoaderPhysPreset.h"
#include "AssetLoaders/AssetLoaderPixelShader.h"
#include "AssetLoaders/AssetLoaderRawFile.h"
#include "AssetLoaders/AssetLoaderScriptFile.h"
#include "AssetLoaders/AssetLoaderSoundAliasList.h"
#include "AssetLoaders/AssetLoaderSoundCurve.h"
#include "AssetLoaders/AssetLoaderStringTable.h"
#include "AssetLoaders/AssetLoaderStructuredDataDef.h"
#include "AssetLoaders/AssetLoaderSurfaceFxTable.h"
#include "AssetLoaders/AssetLoaderTechniqueSet.h"
#include "AssetLoaders/AssetLoaderTracerDef.h"
#include "AssetLoaders/AssetLoaderVehicleDef.h"
#include "AssetLoaders/AssetLoaderVehicleTrack.h"
#include "AssetLoaders/AssetLoaderVertexDecl.h"
#include "AssetLoaders/AssetLoaderVertexShader.h"
#include "AssetLoaders/AssetLoaderWeapon.h"
#include "AssetLoaders/AssetLoaderWeaponAttachment.h"
#include "AssetLoaders/AssetLoaderXAnim.h"
#include "AssetLoaders/AssetLoaderXModel.h"
#include "AssetLoaders/AssetLoaderXModelSurfs.h"
#include "AssetLoading/AssetLoadingManager.h"
#include "Game/IW5/GameIW5.h"
#include "Image/IwiLoader.h"
#include "ObjContainer/IPak/IPak.h"

using namespace IW5;

ObjLoader::ObjLoader()
{
#define REGISTER_ASSET_LOADER(t)                                                                                                                               \
    {                                                                                                                                                          \
        auto l = std::make_unique<t>();                                                                                                                        \
        m_asset_loaders_by_type[l->GetHandlingAssetType()] = std::move(l);                                                                                     \
    }

    REGISTER_ASSET_LOADER(AssetLoaderPhysPreset)
    REGISTER_ASSET_LOADER(AssetLoaderPhysCollmap)
    REGISTER_ASSET_LOADER(AssetLoaderXAnim)
    REGISTER_ASSET_LOADER(AssetLoaderXModelSurfs)
    REGISTER_ASSET_LOADER(AssetLoaderXModel)
    REGISTER_ASSET_LOADER(AssetLoaderMaterial)
    REGISTER_ASSET_LOADER(AssetLoaderPixelShader)
    REGISTER_ASSET_LOADER(AssetLoaderVertexShader)
    REGISTER_ASSET_LOADER(AssetLoaderVertexDecl)
    REGISTER_ASSET_LOADER(AssetLoaderTechniqueSet)
    REGISTER_ASSET_LOADER(AssetLoaderGfxImage)
    REGISTER_ASSET_LOADER(AssetLoaderSoundAliasList)
    REGISTER_ASSET_LOADER(AssetLoaderSoundCurve)
    REGISTER_ASSET_LOADER(AssetLoaderLoadedSound)
    REGISTER_ASSET_LOADER(AssetLoaderClipMap)
    REGISTER_ASSET_LOADER(AssetLoaderComWorld)
    REGISTER_ASSET_LOADER(AssetLoaderGlassWorld)
    REGISTER_ASSET_LOADER(AssetLoaderPathData)
    REGISTER_ASSET_LOADER(AssetLoaderVehicleTrack)
    REGISTER_ASSET_LOADER(AssetLoaderMapEnts)
    REGISTER_ASSET_LOADER(AssetLoaderFxWorld)
    REGISTER_ASSET_LOADER(AssetLoaderGfxWorld)
    REGISTER_ASSET_LOADER(AssetLoaderGfxLightDef)
    REGISTER_ASSET_LOADER(AssetLoaderFont)
    REGISTER_ASSET_LOADER(AssetLoaderMenuList)
    REGISTER_ASSET_LOADER(AssetLoaderMenuDef)
    REGISTER_ASSET_LOADER(AssetLoaderLocalizeEntry)
    REGISTER_ASSET_LOADER(AssetLoaderWeaponAttachment)
    REGISTER_ASSET_LOADER(AssetLoaderWeapon)
    REGISTER_ASSET_LOADER(AssetLoaderFx)
    REGISTER_ASSET_LOADER(AssetLoaderFxImpactTable)
    REGISTER_ASSET_LOADER(AssetLoaderSurfaceFxTable)
    REGISTER_ASSET_LOADER(AssetLoaderRawFile)
    REGISTER_ASSET_LOADER(AssetLoaderScriptFile)
    REGISTER_ASSET_LOADER(AssetLoaderStringTable)
    REGISTER_ASSET_LOADER(AssetLoaderLeaderboard)
    REGISTER_ASSET_LOADER(AssetLoaderStructuredDataDef)
    REGISTER_ASSET_LOADER(AssetLoaderTracerDef)
    REGISTER_ASSET_LOADER(AssetLoaderVehicleDef)
    REGISTER_ASSET_LOADER(AssetLoaderAddonMapEnts)

#undef REGISTER_ASSET_LOADER
}

bool ObjLoader::IsMpZone(const Zone& zone)
{
    return zone.m_name.compare(0, 3, "mp_") == 0 || zone.m_name.compare(zone.m_name.length() - 3, 3, "_mp") == 0;
}

bool ObjLoader::IsZmZone(const Zone& zone)
{
    return zone.m_name.compare(0, 3, "zm_") == 0 || zone.m_name.compare(zone.m_name.length() - 3, 3, "_zm") == 0;
}

void ObjLoader::LoadReferencedContainersForZone(ISearchPath& searchPath, Zone& zone) const {}

void ObjLoader::UnloadContainersOfZone(Zone& zone) const {}

bool ObjLoader::LoadAssetForZone(AssetLoadingContext& context, const asset_type_t assetType, const std::string& assetName) const
{
    AssetLoadingManager assetLoadingManager(m_asset_loaders_by_type, context);
    return assetLoadingManager.LoadAssetFromLoader(assetType, assetName);
}

void ObjLoader::FinalizeAssetsForZone(AssetLoadingContext& context) const
{
    for (const auto& [type, loader] : m_asset_loaders_by_type)
        loader->FinalizeAssetsForZone(context);
}
