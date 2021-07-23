#pragma once

#include <memory>

#include "Writing/IZoneWriterFactory.h"

namespace IW3
{
    class ZoneWriterFactory final : public IZoneWriterFactory
    {
        class Impl;

    public:
        _NODISCARD bool SupportsZone(Zone* zone) const override;
        _NODISCARD std::unique_ptr<ZoneWriter> CreateWriter(Zone* zone) const override;
    };
}