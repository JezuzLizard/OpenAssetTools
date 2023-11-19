#pragma once

#include "Loading/ILoadingStep.h"

#include <memory>

class StepAddProcessor final : public ILoadingStep
{
    std::unique_ptr<StreamProcessor> m_stream_processor;

public:
    explicit StepAddProcessor(std::unique_ptr<StreamProcessor> streamProcessor);

    void PerformStep(ZoneLoader* zoneLoader, ILoadingStream* stream) override;
};
