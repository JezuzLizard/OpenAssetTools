#include "AssetLoaderDDLRoot.h"

#include "Game/T6/T6.h"
#include "Game/T6/DDL/JsonDDLRootLoader.h"
#include "Pool/GlobalAssetPool.h"

#include <cstring>
#include <format>
#include <iostream>

using namespace T6;

void* AssetLoaderWeaponCamo::CreateEmptyAsset(const std::string& assetName, MemoryManager* memory)
{
    auto* weaponCamo = memory->Create<WeaponCamo>();
    memset(weaponCamo, 0, sizeof(WeaponCamo));
    weaponCamo->name = memory->Dup(assetName.c_str());

    return weaponCamo;
}

bool AssetLoaderWeaponCamo::CanLoadFromRaw() const
{
    return true;
}

bool AssetLoaderWeaponCamo::LoadFromRaw(
    const std::string& assetName, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager, Zone* zone) const
{
    return true;
}
