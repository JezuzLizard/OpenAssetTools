#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperScriptParseTree final : public AbstractAssetDumper<ScriptParseTree>
    {
    protected:
        bool ShouldDump(XAssetInfo<ScriptParseTree>* asset) override;
        bool CanDumpAsRaw() override;

        std::string GetFileNameForAsset(Zone* zone, XAssetInfo<ScriptParseTree>* asset) override;
        void DumpRaw(AssetDumpingContext& context, XAssetInfo<ScriptParseTree>* asset, std::ostream& stream) override;
    };
}
