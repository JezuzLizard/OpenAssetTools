#include "AssetLoaderDDLRoot.h"

#include "Game/T6/T6.h"
#include "Game/T6/DDL/JsonDDLRootLoader.h"
#include "Pool/GlobalAssetPool.h"

#include <cstring>
#include <format>
#include <iostream>

using namespace T6;

void* AssetLoaderDDLRoot::CreateEmptyAsset(const std::string& assetName, MemoryManager* memory)
{
    auto* ddlRoot = memory->Create<ddlRoot_t>();
    memset(ddlRoot, 0, sizeof(ddlRoot_t));
    ddlRoot->name = memory->Dup(assetName.c_str());

    return ddlRoot;
}

bool AssetLoaderDDLRoot::CanLoadFromRaw() const
{
    return true;
}

bool AssetLoaderDDLRoot::LoadFromRaw(
    const std::string& assetName, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager, Zone* zone) const
{
    const auto fileName = std::format("{}root.json", assetName);
    const auto file = searchPath->Open(fileName);
    if (!file.IsOpen())
        return false;

    auto* ddlRoot = memory->Alloc<ddlRoot_t>();
    ddlRoot->name = memory->Dup(assetName.c_str());

    std::vector<XAssetInfoGeneric*> dependencies;
    if (LoadDDLRootAsJson(*file.m_stream, *ddlRoot, searchPath, memory, manager, dependencies))
        manager->AddAsset<AssetDDL>(assetName, ddlRoot, std::move(dependencies));
    else
        std::cerr << "Failed to load weapon camo \"" << assetName << "\"\n";

    return true;
}
