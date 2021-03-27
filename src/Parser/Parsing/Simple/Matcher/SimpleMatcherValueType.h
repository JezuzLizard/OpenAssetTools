#pragma once

#include "Parsing/Simple/SimpleParserValue.h"
#include "Parsing/Matcher/AbstractMatcher.h"

class SimpleMatcherValueType final : public AbstractMatcher<SimpleParserValue>
{
    SimpleParserValueType m_type;

protected:
    MatcherResult<SimpleParserValue> CanMatch(ILexer<SimpleParserValue>* lexer, unsigned tokenOffset) override;

public:
    explicit SimpleMatcherValueType(SimpleParserValueType type);
};
