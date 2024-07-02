#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/T6/T6.h"

namespace T6
{
    class AssetDumperMapEnts final : public AbstractAssetDumper<MapEnts>
    {
    private:
        void DumpTriggerModels(std::ostream& stream, const MapTriggers& trigger);
        void DumpTriggerHulls(std::ostream& stream, const MapTriggers& trigger);
        void DumpTriggerSlabs(std::ostream& stream, const MapTriggers& trigger);
    protected:
        bool ShouldDump(XAssetInfo<MapEnts>* asset) override;
        void DumpAsset(AssetDumpingContext& context, XAssetInfo<MapEnts>* asset) override;
    };
}