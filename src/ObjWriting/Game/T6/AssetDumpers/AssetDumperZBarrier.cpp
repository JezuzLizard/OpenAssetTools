#include "AssetDumperZBarrier.h"

#include <cassert>
#include <type_traits>

#include "Game/T6/ObjConstantsT6.h"
#include "Game/T6/InfoString/InfoStringFromStructConverter.h"
#include "Game/T6/InfoString/ZBarrierFields.h"

using namespace T6;

namespace T6
{
    class InfoStringFromZBarrierConverter final : public InfoStringFromStructConverter
    {
    protected:
        void FillFromExtensionField(const cspField_t& field) override
        {
            assert(false);
        }

    public:
        InfoStringFromZBarrierConverter(const ZBarrierDef* structure, const cspField_t* fields, const size_t fieldCount, std::function<std::string(scr_string_t)> scriptStringValueCallback)
            : InfoStringFromStructConverter(structure, fields, fieldCount, std::move(scriptStringValueCallback))
        {
        }
    };
}

InfoString AssetDumperZBarrier::CreateInfoString(XAssetInfo<ZBarrierDef>* asset)
{
    InfoStringFromZBarrierConverter converter(asset->Asset(), zbarrier_fields, std::extent<decltype(zbarrier_fields)>::value, [asset](const scr_string_t scrStr) -> std::string
    {
        assert(scrStr < asset->m_zone->m_script_strings.Count());
        if (scrStr >= asset->m_zone->m_script_strings.Count())
            return "";

        return asset->m_zone->m_script_strings[scrStr];
    });

    return converter.Convert();
}

bool AssetDumperZBarrier::ShouldDump(XAssetInfo<ZBarrierDef>* asset)
{
    return true;
}

bool AssetDumperZBarrier::CanDumpAsRaw()
{
    return true;
}

bool AssetDumperZBarrier::CanDumpAsGdtEntry()
{
    return true;
}

std::string AssetDumperZBarrier::GetFileNameForAsset(Zone* zone, XAssetInfo<ZBarrierDef>* asset)
{
    return "zbarrier/" + asset->m_name;
}

GdtEntry AssetDumperZBarrier::DumpGdtEntry(AssetDumpingContext& context, XAssetInfo<ZBarrierDef>* asset)
{
    const auto infoString = CreateInfoString(asset);
    GdtEntry gdtEntry(asset->m_name, ObjConstants::GDF_FILENAME_ZBARRIER);
    infoString.ToGdtProperties(ObjConstants::INFO_STRING_PREFIX_ZBARRIER, gdtEntry);

    return gdtEntry;
}

void AssetDumperZBarrier::DumpRaw(AssetDumpingContext& context, XAssetInfo<ZBarrierDef>* asset, std::ostream& stream)
{
    const auto infoString = CreateInfoString(asset);
    const auto stringValue = infoString.ToString(ObjConstants::INFO_STRING_PREFIX_ZBARRIER);
    stream.write(stringValue.c_str(), stringValue.size());
}
