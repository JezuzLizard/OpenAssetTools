#pragma once
#include "Techset/Parsing/TechniqueFileParser.h"

#include <vector>

namespace techset
{
    class TechniqueNoScopeSequences
    {
        TechniqueNoScopeSequences() = default;

    public:
        static const std::vector<TechniqueParser::sequence_t*>& GetSequences();
    };
} // namespace techset
