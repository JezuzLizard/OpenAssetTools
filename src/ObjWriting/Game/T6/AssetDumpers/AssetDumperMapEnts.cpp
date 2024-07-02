#define NOMINMAX
#include "AssetDumperMapEnts.h"

#include <algorithm>

using namespace T6;

bool AssetDumperMapEnts::ShouldDump(XAssetInfo<MapEnts>* asset)
{
    return true;
}

void AssetDumperMapEnts::DumpTriggerModels(std::ostream& stream, const MapTriggers& trigger)
{
    // Write count
    stream.write(reinterpret_cast<char*>(trigger.count), 1);

    // Write models
    for (auto i = 0u; i < trigger.count; i++)
    {
        stream.write(reinterpret_cast<char*>(trigger.models[i].contents), 1);
        stream.write(reinterpret_cast<char*>(trigger.models[i].hullCount), 1);
        stream.write(reinterpret_cast<char*>(trigger.models[i].firstHull), 1);
    }
}

void AssetDumperMapEnts::DumpTriggerHulls(std::ostream& stream, const MapTriggers& trigger)
{
    // Write count
    stream.write(reinterpret_cast<char*>(trigger.hullCount), 1);

    // Write hulls
    for (auto i = 0u; i < trigger.count; i++)
    {
        stream.write(reinterpret_cast<char*>(trigger.hulls[i].bounds.midPoint.v), 3);
        stream.write(reinterpret_cast<char*>(trigger.hulls[i].bounds.halfSize.v), 3);
        
        stream.write(reinterpret_cast<char*>(trigger.hulls[i].contents), 1);
        stream.write(reinterpret_cast<char*>(trigger.hulls[i].slabCount), 1);
        stream.write(reinterpret_cast<char*>(trigger.hulls[i].firstSlab), 1);
    }
}

void AssetDumperMapEnts::DumpTriggerSlabs(std::ostream& stream, const MapTriggers& trigger)
{
    // Write count
    stream.write(reinterpret_cast<char*>(trigger.slabCount), 1);

    // Write models
    for (auto i = 0u; i < trigger.count; i++)
    {
        stream.write(reinterpret_cast<char*>(trigger.models[i].contents), 1);
        stream.write(reinterpret_cast<char*>(trigger.models[i].hullCount), 1);
        stream.write(reinterpret_cast<char*>(trigger.models[i].firstHull), 1);
    }
}

void AssetDumperMapEnts::DumpAsset(AssetDumpingContext& context, XAssetInfo<MapEnts>* asset)
{
    const auto* mapEnts = asset->Asset();
    const auto mapentsStringAssetFile = context.OpenAssetFile(asset->m_name + ".mapents");
    const auto mapentsTriggersAssetFile = context.OpenAssetFile(asset->m_name + ".triggers");

    if (!mapentsStringAssetFile)
        return;

    if (!mapentsTriggersAssetFile)
        return;

    auto& mapEntsStringStream = *mapentsStringAssetFile;

    mapEntsStringStream.write(mapEnts->entityString, std::max(mapEnts->numEntityChars - 1, 0));

    auto& mapEntsTriggerStream = *mapentsTriggersAssetFile;

    auto& trigger = mapEnts->trigger;

    AssetDumperMapEnts::DumpTriggerModels(mapEntsTriggerStream, trigger);
}