#pragma once

#include "Zone/Definition/Parsing/ZoneDefinitionParser.h"

class SequenceZoneDefinitionEntry final : public ZoneDefinitionParser::sequence_t
{
    static constexpr auto TAG_REFERENCE = 1;

    static constexpr auto CAPTURE_TYPE_NAME = 1;
    static constexpr auto CAPTURE_ASSET_NAME = 2;

protected:
    void ProcessMatch(ZoneDefinitionParserState* state, SequenceResult<ZoneDefinitionParserValue>& result) const override;

public:
    SequenceZoneDefinitionEntry();
};
