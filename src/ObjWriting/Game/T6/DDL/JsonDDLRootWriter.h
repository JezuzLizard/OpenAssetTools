#pragma once

#include "Dumping/AssetDumpingContext.h"
#include "Game/T6/T6.h"

#include <ostream>

namespace T6
{
    void DumpDDLRootAsJson(std::ostream& stream, AssetDumpingContext& context, const ddlRoot_t* ddlRoot);
} // namespace T6
