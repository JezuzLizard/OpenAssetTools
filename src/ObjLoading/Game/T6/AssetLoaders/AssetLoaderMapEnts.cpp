#include "AssetLoaderMapEnts.h"

#include <cstring>

#include "ObjLoading.h"
#include "Game/T6/T6.h"
#include "Pool/GlobalAssetPool.h"

using namespace T6;

void* AssetLoaderMapEnts::CreateEmptyAsset(const std::string& assetName, MemoryManager* memory)
{
    auto* mapEnts = memory->Create<MapEnts>();
    memset(mapEnts, 0, sizeof(MapEnts));
    mapEnts->name = memory->Dup(assetName.c_str());
    return mapEnts;
}

bool AssetLoaderMapEnts::CanLoadFromRaw() const
{
    return true;
}

bool AssetLoaderMapEnts::LoadFromRaw(const std::string& assetName, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager, Zone* zone) const
{
    const auto file = searchPath->Open(assetName);
    if (!file.IsOpen())
        return false;

    auto* mapEnts = memory->Create<MapEnts>();
    mapEnts->name = memory->Dup(assetName.c_str());
    mapEnts->numEntityChars = static_cast<int>(file.m_length);

    auto* fileBuffer = static_cast<char*>(memory->Alloc(static_cast<size_t>(file.m_length + 1)));
    file.m_stream->read(fileBuffer, file.m_length);
    if (file.m_stream->gcount() != file.m_length)
        return false;
    fileBuffer[mapEnts->numEntityChars] = '\0';

    mapEnts->entityString = static_cast<char16*>(fileBuffer);
    manager->AddAsset(ASSET_TYPE_MAP_ENTS, assetName, mapEnts);

    return true;
}