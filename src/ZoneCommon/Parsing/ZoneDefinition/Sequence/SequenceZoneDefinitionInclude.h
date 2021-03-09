#pragma once

#include "Parsing/ZoneDefinition/ZoneDefinitionParser.h"

class SequenceZoneDefinitionInclude final : public ZoneDefinitionParser::sequence_t
{
    static constexpr auto CAPTURE_INCLUDE_NAME = 1;

protected:
    void ProcessMatch(ZoneDefinition* state, SequenceResult<SimpleParserValue>& result) const override;

public:
    SequenceZoneDefinitionInclude();
};
