#pragma once

#include "ContentLister/ZoneDefWriter.h"

namespace IW4
{
    class ZoneDefWriter final : public AbstractZoneDefWriter
    {
    protected:
        void WriteMetaData(::ZoneDefinitionOutputStream& stream, ::Zone* zone) const override;
        void WriteContent(::ZoneDefinitionOutputStream& stream, ::Zone* zone) const override;

    public:
        bool CanHandleZone(Zone* zone) const override;
    };
}