#pragma once

#include "Parsing/Impl/AbstractLexer.h"
#include "SimpleParserValue.h"

#include <cstdint>
#include <limits>
#include <memory>

class SimpleLexer final : public AbstractLexer<SimpleParserValue>
{
public:
    class Config
    {
    public:
        class MultiCharacterToken
        {
        public:
            int m_id;
            std::string m_value;

            MultiCharacterToken(int id, std::string value);
        };

        bool m_emit_new_line_tokens = false;
        bool m_read_strings = true;
        bool m_string_escape_sequences = false;
        bool m_read_integer_numbers = true;
        bool m_read_floating_point_numbers = true;
        std::vector<MultiCharacterToken> m_multi_character_tokens;
    };

    explicit SimpleLexer(IParserLineStream* stream);
    SimpleLexer(IParserLineStream* stream, Config config);

protected:
    class MultiCharacterTokenLookupEntry
    {
    public:
        int m_id;
        std::string m_value;
        std::unique_ptr<MultiCharacterTokenLookupEntry> m_next;

        MultiCharacterTokenLookupEntry(int id, std::string value);
    };

    std::unique_ptr<MultiCharacterTokenLookupEntry> m_multi_character_token_lookup[std::numeric_limits<uint8_t>::max() + 1];

    void AddMultiCharacterTokenConfigToLookup(Config::MultiCharacterToken tokenConfig);
    bool ReadMultiCharacterToken(const MultiCharacterTokenLookupEntry* multiTokenLookup);

    SimpleParserValue GetNextToken() override;

    Config m_config;
    bool m_check_for_multi_character_tokens;
    size_t m_last_line;
};
