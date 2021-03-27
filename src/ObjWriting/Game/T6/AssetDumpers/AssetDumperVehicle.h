#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"
#include "InfoString/InfoString.h"

namespace T6
{
    class AssetDumperVehicle final : public AbstractAssetDumper<VehicleDef>
    {
        static InfoString CreateInfoString(XAssetInfo<VehicleDef>* asset);

    protected:
        bool ShouldDump(XAssetInfo<VehicleDef>* asset) override;
        bool CanDumpAsRaw() override;
        bool CanDumpAsGdtEntry() override;

        std::string GetFileNameForAsset(Zone* zone, XAssetInfo<VehicleDef>* asset) override;
        GdtEntry DumpGdtEntry(AssetDumpingContext& context, XAssetInfo<VehicleDef>* asset) override;
        void DumpRaw(AssetDumpingContext& context, XAssetInfo<VehicleDef>* asset, std::ostream& stream) override;
    };
}
