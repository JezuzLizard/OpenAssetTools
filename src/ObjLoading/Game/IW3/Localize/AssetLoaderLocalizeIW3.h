#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW3/IW3.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"
#include "Zone/Zone.h"

#include <memory>

namespace IW3
{
    std::unique_ptr<AssetCreator<AssetLocalize>> CreateLocalizeLoader(MemoryManager& memory, ISearchPath& searchPath, Zone& zone);
} // namespace IW3