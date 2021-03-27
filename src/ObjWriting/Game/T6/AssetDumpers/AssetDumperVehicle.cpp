#include "AssetDumperVehicle.h"

#include <cassert>
#include <type_traits>

#include "Game/T6/ObjConstantsT6.h"
#include "Game/T6/InfoString/EnumStrings.h"
#include "Game/T6/InfoString/InfoStringFromStructConverter.h"
#include "Game/T6/InfoString/VehicleFields.h"

using namespace T6;

namespace T6
{
    class InfoStringFromVehicleConverter final : public InfoStringFromStructConverter
    {
    protected:
        void FillFromExtensionField(const cspField_t& field) override
        {
            switch (static_cast<VehicleFieldType>(field.iFieldType))
            {
            case VFT_TYPE:
                FillFromEnumInt(std::string(field.szName), field.iOffset, s_vehicleClassNames, std::extent<decltype(s_vehicleClassNames)>::value);
                break;

            case VFT_CAMERAMODE:
                FillFromEnumInt(std::string(field.szName), field.iOffset, s_vehicleCameraModes, std::extent<decltype(s_vehicleCameraModes)>::value);
                break;

            case VFT_TRACTION_TYPE:
                FillFromEnumInt(std::string(field.szName), field.iOffset, s_tractionTypeNames, std::extent<decltype(s_tractionTypeNames)>::value);
                break;

            case VFT_MPH_TO_INCHES_PER_SECOND:
                {
                    const auto* num = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(m_structure) + field.iOffset);
                    m_info_string.SetValueForKey(std::string(field.szName), std::to_string(*num / 17.6f));
                    break;
                }

            case VFT_POUNDS_TO_GAME_MASS:
                {
                    const auto* num = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(m_structure) + field.iOffset);
                    m_info_string.SetValueForKey(std::string(field.szName), std::to_string(*num / 0.001f));
                    break;
                }

            case VFT_TEAM:
                {
                    const auto* num = reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_structure) + field.iOffset);
                    switch (*num)
                    {
                    case TEAM_AXIS:
                        m_info_string.SetValueForKey(std::string(field.szName), "axis");
                        break;

                    case TEAM_ALLIES:
                        m_info_string.SetValueForKey(std::string(field.szName), "allies");
                        break;

                    default:
                        assert(false);
                        m_info_string.SetValueForKey(std::string(field.szName), "");
                        break;
                    }
                    break;
                }

            case VFT_KEY_BINDING:
            case VFT_GRAPH:
            case VFT_WIIUCONTROLOVERRIDE:
            case VFT_NUM:
            default:
                assert(false);
                break;
            }
        }

    public:
        InfoStringFromVehicleConverter(const VehicleDef* structure, const cspField_t* fields, const size_t fieldCount, std::function<std::string(scr_string_t)> scriptStringValueCallback)
            : InfoStringFromStructConverter(structure, fields, fieldCount, std::move(scriptStringValueCallback))
        {
        }
    };
}

InfoString AssetDumperVehicle::CreateInfoString(XAssetInfo<VehicleDef>* asset)
{
    InfoStringFromVehicleConverter converter(asset->Asset(), vehicle_fields, std::extent<decltype(vehicle_fields)>::value, [asset](const scr_string_t scrStr) -> std::string
    {
        assert(scrStr < asset->m_zone->m_script_strings.Count());
        if (scrStr >= asset->m_zone->m_script_strings.Count())
            return "";

        return asset->m_zone->m_script_strings[scrStr];
    });

    return converter.Convert();
}

bool AssetDumperVehicle::ShouldDump(XAssetInfo<VehicleDef>* asset)
{
    return true;
}

bool AssetDumperVehicle::CanDumpAsRaw()
{
    return true;
}

bool AssetDumperVehicle::CanDumpAsGdtEntry()
{
    return true;
}

std::string AssetDumperVehicle::GetFileNameForAsset(Zone* zone, XAssetInfo<VehicleDef>* asset)
{
    return "vehicles/" + asset->m_name;
}

GdtEntry AssetDumperVehicle::DumpGdtEntry(AssetDumpingContext& context, XAssetInfo<VehicleDef>* asset)
{
    const auto infoString = CreateInfoString(asset);
    GdtEntry gdtEntry(asset->m_name, ObjConstants::GDF_FILENAME_VEHICLE);
    infoString.ToGdtProperties(ObjConstants::INFO_STRING_PREFIX_VEHICLE, gdtEntry);

    return gdtEntry;
}

void AssetDumperVehicle::DumpRaw(AssetDumpingContext& context, XAssetInfo<VehicleDef>* asset, std::ostream& stream)
{
    const auto infoString = CreateInfoString(asset);
    const auto stringValue = infoString.ToString(ObjConstants::INFO_STRING_PREFIX_VEHICLE);
    stream.write(stringValue.c_str(), stringValue.size());
}
