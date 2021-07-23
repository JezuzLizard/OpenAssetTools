#include "AssetLoaderRawFile.h"

#include <cstring>

#include "Game/IW5/IW5.h"
#include "Pool/GlobalAssetPool.h"

using namespace IW5;

void* AssetLoaderRawFile::CreateEmptyAsset(const std::string& assetName, MemoryManager* memory)
{
    auto* rawFile = memory->Create<RawFile>();
    memset(rawFile, 0, sizeof(RawFile));
    rawFile->name = memory->Dup(assetName.c_str());
    return rawFile;
}

bool AssetLoaderRawFile::CanLoadFromRaw() const
{
    return true;
}

bool AssetLoaderRawFile::LoadFromRaw(const std::string& assetName, ISearchPath* searchPath, MemoryManager* memory, IAssetLoadingManager* manager, Zone* zone) const
{
    const auto file = searchPath->Open(assetName);
    if (!file.IsOpen())
        return false;

    auto* rawFile = memory->Create<RawFile>();
    rawFile->name = memory->Dup(assetName.c_str());
    rawFile->compressedLen = static_cast<int>(file.m_length + 1);

    auto* fileBuffer = static_cast<char*>(memory->Alloc(static_cast<size_t>(file.m_length + 1)));
    file.m_stream->read(fileBuffer, file.m_length);
    if (file.m_stream->gcount() != file.m_length)
        return false;
    fileBuffer[file.m_length] = '\0';

    rawFile->buffer = fileBuffer;
    manager->AddAsset(ASSET_TYPE_RAWFILE, assetName, rawFile);

    return true;
}