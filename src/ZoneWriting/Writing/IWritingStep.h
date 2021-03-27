#pragma once

#include "Writing/ZoneWriter.h"
#include "IWritingStream.h"

class ZoneWriter;

class IWritingStep
{
public:
    IWritingStep() = default;
    virtual ~IWritingStep() = default;
    IWritingStep(const IWritingStep& other) = default;
    IWritingStep(IWritingStep&& other) noexcept = default;
    IWritingStep& operator=(const IWritingStep& other) = default;
    IWritingStep& operator=(IWritingStep&& other) noexcept = default;

    virtual void PerformStep(ZoneWriter* zoneWriter, IWritingStream* stream) = 0;
};
