#pragma once

#include "AssetLoading/IAssetLoadingManager.h"
#include "Game/T6/T6.h"
#include "Utils/MemoryManager.h"

#include <istream>

namespace T6
{
    bool LoadDDLRootAsJson(std::istream& stream,
                           ddlRoot_t& ddlRoot,
                           ISearchPath* searchPath,
                           MemoryManager* memory,
                           IAssetLoadingManager* manager,
                           std::vector<XAssetInfoGeneric*>& dependencies);
} // namespace T6
