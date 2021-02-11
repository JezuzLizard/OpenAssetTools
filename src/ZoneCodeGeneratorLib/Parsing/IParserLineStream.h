#pragma once

#include <string>

#include "Utils/ClassUtils.h"

class ParserLine
{
    static const std::string EMPTY_STRING;

public:
    std::reference_wrapper<const std::string> m_filename;
    int m_line_number;
    std::string m_line;

    ParserLine();
    ParserLine(const std::string& filename, int lineNumber, std::string line);

    _NODISCARD bool IsEof() const;
};

class IParserLineStream
{
public:
    IParserLineStream() = default;
    virtual ~IParserLineStream() = default;

    IParserLineStream(const IParserLineStream& other) = default;
    IParserLineStream(IParserLineStream&& other) noexcept = default;
    IParserLineStream& operator=(const IParserLineStream& other) = default;
    IParserLineStream& operator=(IParserLineStream&& other) noexcept = default;

    virtual ParserLine NextLine() = 0;
    virtual bool IncludeFile(const std::string& filename) = 0;
    _NODISCARD virtual bool IsOpen() const = 0;
    _NODISCARD virtual bool Eof() const = 0;
};
