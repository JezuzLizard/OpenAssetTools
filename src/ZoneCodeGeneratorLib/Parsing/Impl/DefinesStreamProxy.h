#pragma once

#include <unordered_map>
#include <stack>

#include "AbstractDirectiveStreamProxy.h"
#include "Parsing/IParserLineStream.h"

class DefinesStreamProxy final : public AbstractDirectiveStreamProxy
{
    static constexpr const char* DEFINE_DIRECTIVE = "define ";
    static constexpr const char* UNDEF_DIRECTIVE = "undef ";
    static constexpr const char* IFDEF_DIRECTIVE = "ifdef ";
    static constexpr const char* IFNDEF_DIRECTIVE = "ifndef ";
    static constexpr const char* ELSE_DIRECTIVE = "else";
    static constexpr const char* ENDIF_DIRECTIVE = "endif";

public:
    class DefineParameterPosition
    {
    public:
        unsigned m_parameter_index;
        unsigned m_parameter_position;

        DefineParameterPosition();
        DefineParameterPosition(unsigned index, unsigned position);
    };

    class Define
    {
    public:
        std::string m_name;
        std::string m_value;
        std::vector<DefineParameterPosition> m_parameter_positions;

        Define();
        Define(std::string name, std::string value);
        void IdentifyParameters(std::vector<std::string>& parameterNames);
        std::string Render(std::vector<std::string>& parameterValues);
    };

private:
    IParserLineStream* const m_stream;
    std::unordered_map<std::string, Define> m_defines;
    std::stack<bool> m_modes;
    unsigned m_ignore_depth;
    
    std::vector<std::string> MatchDefineParameters(const ParserLine& line, unsigned& parameterPosition);
    _NODISCARD bool MatchDefineDirective(const ParserLine& line, unsigned directivePosition);
    _NODISCARD bool MatchUndefDirective(const ParserLine& line, unsigned directivePosition);
    _NODISCARD bool MatchIfdefDirective(const ParserLine& line, unsigned directivePosition);
    _NODISCARD bool MatchElseDirective(const ParserLine& line, unsigned directivePosition);
    _NODISCARD bool MatchEndifDirective(const ParserLine& line, unsigned directivePosition);
    _NODISCARD bool MatchDirectives(const ParserLine& line);
    
    void ExtractParametersFromDefineUsage(const ParserLine& line, unsigned parameterStart, unsigned& parameterEnd, std::vector<std::string>& parameterValues);
    bool FindDefineForWord(const ParserLine& line, unsigned wordStart, unsigned wordEnd, Define*& value);
    void ExpandDefines(ParserLine& line);

public:
    explicit DefinesStreamProxy(IParserLineStream* stream);

    void AddDefine(Define define);
    void Undefine(const std::string& name);

    ParserLine NextLine() override;
    bool IncludeFile(const std::string& filename) override;
    void PopCurrentFile() override;
    _NODISCARD bool IsOpen() const override;
    _NODISCARD bool Eof() const override;
};
