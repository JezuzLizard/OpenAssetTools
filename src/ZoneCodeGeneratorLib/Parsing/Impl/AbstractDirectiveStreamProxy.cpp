#include "AbstractDirectiveStreamProxy.h"

TokenPos AbstractDirectiveStreamProxy::CreatePos(const ParserLine& line, const unsigned position)
{
    return TokenPos(line.m_filename.get(), line.m_line_number, static_cast<int>(position + 1));
}

bool AbstractDirectiveStreamProxy::SkipWhitespace(const ParserLine& line, unsigned& position)
{
    while(true)
    {
        if (position >= line.m_line.size())
            return false;

        if (isspace(line.m_line[position]))
            position++;
        else
            break;
    }

    return true;
}

bool AbstractDirectiveStreamProxy::ExtractInteger(const ParserLine& line, unsigned& position, int& value)
{
    if (position >= line.m_line.size())
        return false;

    const auto* startPosition = &line.m_line[position];
    char* endPosition;
    value = strtol(startPosition, &endPosition, 0);
    const auto len = endPosition - startPosition;

    if(len > 0)
    {
        position += len;
        return true;
    }

    return false;
}

bool AbstractDirectiveStreamProxy::ExtractIdentifier(const ParserLine& line, unsigned& position)
{
    auto firstChar = true;
    while (true)
    {
        if (position >= line.m_line.size())
            return !firstChar;

        const auto c = line.m_line[position];
        if (isalpha(c) 
            || c == '_' 
            || firstChar && isdigit(c))
        {
            position++;
        }
        else
            return !firstChar;

        firstChar = false;
    }
}

bool AbstractDirectiveStreamProxy::MatchCharacter(const ParserLine& line, unsigned& position, char c)
{
    if (position < line.m_line.size() && line.m_line[position] == c)
    {
        position++;
        return true;
    }

    return false;
}

bool AbstractDirectiveStreamProxy::MatchNextCharacter(const ParserLine& line, unsigned& position, char c)
{
    return SkipWhitespace(line, position) && MatchCharacter(line, position, c);
}

bool AbstractDirectiveStreamProxy::MatchString(const ParserLine& line, unsigned& position, const char* str, unsigned len)
{
    if (line.m_line.compare(position, len, str) == 0)
    {
        position += len;
        return true;
    }

    return false;
}

bool AbstractDirectiveStreamProxy::MatchNextString(const ParserLine& line, unsigned& position, const char* str, unsigned len)
{
    return SkipWhitespace(line, position) && MatchString(line, position, str, len);
}

bool AbstractDirectiveStreamProxy::FindDirective(const ParserLine& line, unsigned& directivePosition)
{
    directivePosition = 0;
    for (; directivePosition < line.m_line.size(); directivePosition++)
    {
        const auto c = line.m_line[directivePosition];

        if (isspace(c))
            continue;

        return c == '#';
    }

    return false;
}
