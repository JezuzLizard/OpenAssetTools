#pragma once

#include <iterator>
#include <memory>

#include "Parsing/IParserValue.h"
#include "AbstractMatcher.h"

template <typename TokenType>
class MatcherOr final : public AbstractMatcher<TokenType>
{
    // TokenType must inherit IParserValue
    static_assert(std::is_base_of<IParserValue, TokenType>::value);

    std::vector<std::unique_ptr<AbstractMatcher<TokenType>>> m_matchers;

protected:
    MatcherResult<TokenType> CanMatch(AbstractLexer<TokenType>* lexer, unsigned tokenOffset) override
    {
        for (const auto& matcher : m_matchers)
        {
            const auto result = matcher->Match(lexer, tokenOffset);

            if (!result.m_matches)
                continue;

            return result;
        }

        return MatcherResult<TokenType>::NoMatch();
    }

public:
    MatcherOr(std::initializer_list<Movable<std::unique_ptr<AbstractMatcher<TokenType>>>> matchers)
        : m_matchers(std::make_move_iterator(matchers.begin()), std::make_move_iterator(matchers.end()))
    {
    }
};
