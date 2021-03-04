#include "gfximage_actions.h"

#include <cassert>
#include <cstring>

using namespace IW4;

Actions_GfxImage::Actions_GfxImage(Zone* zone)
    : AssetLoadingActions(zone)
{
}

void Actions_GfxImage::OnImageLoaded(GfxImage* image) const
{
    image->cardMemory.platform[0] = 0;
}

void Actions_GfxImage::LoadImageData(GfxImageLoadDef* loadDef, GfxImage* image) const
{
    if (loadDef->resourceSize > 0)
    {
        const size_t loadDefSize = offsetof(IW4::GfxImageLoadDef, data) + loadDef->resourceSize;

        image->texture.loadDef = static_cast<GfxImageLoadDef*>(m_zone->GetMemory()->Alloc(loadDefSize));
        memcpy(image->texture.loadDef, loadDef, loadDefSize);
    }
    else
    {
        image->texture.loadDef = nullptr;
    }
}
