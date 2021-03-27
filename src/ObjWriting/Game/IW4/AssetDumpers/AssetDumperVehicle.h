#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4/IW4.h"
#include "InfoString/InfoString.h"

namespace IW4
{
    class AssetDumperVehicle final : public AbstractAssetDumper<VehicleDef>
    {
        static constexpr const char* FILE_TYPE_STR = "VEHICLEFILE";
        static constexpr const char* GDF_NAME = "vehicle.gdf";
        static cspField_t vehicle_fields[];

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
