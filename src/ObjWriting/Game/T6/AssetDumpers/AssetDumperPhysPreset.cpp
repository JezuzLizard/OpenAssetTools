#include "AssetDumperPhysPreset.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <type_traits>

#include "Game/T6/ObjConstantsT6.h"
#include "Game/T6/InfoString/InfoStringFromStructConverter.h"
#include "Game/T6/InfoString/PhysPresetFields.h"

using namespace T6;

namespace T6
{
    class InfoStringFromPhysPresetConverter final : public InfoStringFromStructConverter
    {
    protected:
        void FillFromExtensionField(const cspField_t& field) override
        {
            assert(false);
        }

    public:
        InfoStringFromPhysPresetConverter(const PhysPresetInfo* structure, const cspField_t* fields, const size_t fieldCount, std::function<std::string(scr_string_t)> scriptStringValueCallback)
            : InfoStringFromStructConverter(structure, fields, fieldCount, std::move(scriptStringValueCallback))
        {
        }
    };
}

void AssetDumperPhysPreset::CopyToPhysPresetInfo(const PhysPreset* physPreset, PhysPresetInfo* physPresetInfo)
{
    physPresetInfo->mass = std::clamp(physPreset->mass * 1000.0f, 1.0f, 2000.0f);
    physPresetInfo->bounce = physPreset->bounce;

    if(std::isinf(physPreset->friction))
    {
        physPresetInfo->isFrictionInfinity = 1;
        physPresetInfo->friction = 0;
    }
    else
    {
        physPresetInfo->isFrictionInfinity = 0;
        physPresetInfo->friction = physPreset->friction;
    }

    physPresetInfo->bulletForceScale = physPreset->bulletForceScale;
    physPresetInfo->explosiveForceScale = physPreset->explosiveForceScale;
    physPresetInfo->piecesSpreadFraction = physPreset->piecesSpreadFraction;
    physPresetInfo->piecesUpwardVelocity = physPreset->piecesUpwardVelocity;
    physPresetInfo->canFloat = physPreset->canFloat;
    physPresetInfo->gravityScale = std::clamp(physPreset->gravityScale, 0.01f, 10.0f);
    physPresetInfo->centerOfMassOffset = physPreset->centerOfMassOffset;
    physPresetInfo->buoyancyBoxMin = physPreset->buoyancyBoxMin;
    physPresetInfo->buoyancyBoxMax = physPreset->buoyancyBoxMax;
}

InfoString AssetDumperPhysPreset::CreateInfoString(XAssetInfo<PhysPreset>* asset)
{
    auto* physPresetInfo = new PhysPresetInfo;
    CopyToPhysPresetInfo(asset->Asset(), physPresetInfo);

    InfoStringFromPhysPresetConverter converter(physPresetInfo, phys_preset_fields, std::extent<decltype(phys_preset_fields)>::value, [asset](const scr_string_t scrStr) -> std::string
        {
            assert(scrStr < asset->m_zone->m_script_strings.Count());
            if (scrStr >= asset->m_zone->m_script_strings.Count())
                return "";

            return asset->m_zone->m_script_strings[scrStr];
        });

    return converter.Convert();
}

bool AssetDumperPhysPreset::ShouldDump(XAssetInfo<PhysPreset>* asset)
{
    return true;
}

bool AssetDumperPhysPreset::CanDumpAsRaw()
{
    return true;
}

bool AssetDumperPhysPreset::CanDumpAsGdtEntry()
{
    return true;
}

std::string AssetDumperPhysPreset::GetFileNameForAsset(Zone* zone, XAssetInfo<PhysPreset>* asset)
{
    return "physic/" + asset->m_name;
}

GdtEntry AssetDumperPhysPreset::DumpGdtEntry(AssetDumpingContext& context, XAssetInfo<PhysPreset>* asset)
{
    const auto infoString = CreateInfoString(asset);
    GdtEntry gdtEntry(asset->m_name, ObjConstants::GDF_FILENAME_PHYS_PRESET);
    infoString.ToGdtProperties(ObjConstants::INFO_STRING_PREFIX_PHYS_PRESET, gdtEntry);

    return gdtEntry;
}

void AssetDumperPhysPreset::DumpRaw(AssetDumpingContext& context, XAssetInfo<PhysPreset>* asset, std::ostream& stream)
{
    const auto infoString = CreateInfoString(asset);
    const auto stringValue = infoString.ToString(ObjConstants::INFO_STRING_PREFIX_PHYS_PRESET);
    stream.write(stringValue.c_str(), stringValue.size());
}