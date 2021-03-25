#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"
#include "InfoString/InfoString.h"

namespace T6
{
    class AssetDumperZBarrier final : public AbstractAssetDumper<ZBarrierDef>
    {
        static InfoString CreateInfoString(XAssetInfo<ZBarrierDef>* asset);

    protected:
        bool ShouldDump(XAssetInfo<ZBarrierDef>* asset) override;
        bool CanDumpAsRaw() override;
        bool CanDumpAsGdtEntry() override;

        std::string GetFileNameForAsset(Zone* zone, XAssetInfo<ZBarrierDef>* asset) override;
        GdtEntry DumpGdtEntry(AssetDumpingContext& context, XAssetInfo<ZBarrierDef>* asset) override;
        void DumpRaw(AssetDumpingContext& context, XAssetInfo<ZBarrierDef>* asset, std::ostream& stream) override;
    };
}
