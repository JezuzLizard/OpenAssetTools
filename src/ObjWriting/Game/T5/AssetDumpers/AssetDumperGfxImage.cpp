#include "AssetDumperGfxImage.h"

#include "Image/DdsWriter.h"
#include "Image/IwiWriter27.h"
#include "ObjWriting.h"

#include <cassert>

using namespace T5;

AssetDumperGfxImage::AssetDumperGfxImage()
{
    switch (ObjWriting::Configuration.ImageOutputFormat)
    {
    case ObjWriting::Configuration_t::ImageOutputFormat_e::DDS:
        m_writer = std::make_unique<DdsWriter>();
        break;
    case ObjWriting::Configuration_t::ImageOutputFormat_e::IWI:
        m_writer = std::make_unique<iwi27::IwiWriter>();
        break;
    default:
        assert(false);
        m_writer = nullptr;
        break;
    }
}

bool AssetDumperGfxImage::ShouldDump(XAssetInfo<GfxImage>* asset)
{
    const auto* image = asset->Asset();
    return image->loadedSize > 0;
}

std::string AssetDumperGfxImage::GetAssetFileName(XAssetInfo<GfxImage>* asset) const
{
    std::string cleanAssetName = asset->m_name;
    for (auto& c : cleanAssetName)
    {
        switch (c)
        {
        case '*':
            c = '_';
            break;

        default:
            break;
        }
    }

    return "images/" + cleanAssetName + m_writer->GetFileExtension();
}

void AssetDumperGfxImage::DumpAsset(AssetDumpingContext& context, XAssetInfo<GfxImage>* asset)
{
    const auto* image = asset->Asset();
    const auto assetFile = context.OpenAssetFile(GetAssetFileName(asset));

    if (!assetFile)
        return;

    auto& stream = *assetFile;
    m_writer->DumpImage(stream, image->texture.texture);
}
