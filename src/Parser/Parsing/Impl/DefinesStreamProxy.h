#pragma once

#include "AbstractDirectiveStreamProxy.h"
#include "Parsing/IParserLineStream.h"
#include "Parsing/Simple/Expression/ISimpleExpression.h"

#include <cstddef>
#include <map>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

class DefinesStreamProxy final : public AbstractDirectiveStreamProxy
{
    static constexpr const char* DEFINE_DIRECTIVE = "define";
    static constexpr const char* UNDEF_DIRECTIVE = "undef";
    static constexpr const char* IF_DIRECTIVE = "if";
    static constexpr const char* ELIF_DIRECTIVE = "elif";
    static constexpr const char* IFDEF_DIRECTIVE = "ifdef";
    static constexpr const char* IFNDEF_DIRECTIVE = "ifndef";
    static constexpr const char* ELSE_DIRECTIVE = "else";
    static constexpr const char* ENDIF_DIRECTIVE = "endif";
    static constexpr const char* DEFINED_KEYWORD = "defined";

    static constexpr auto MAX_DEFINE_ITERATIONS = 128u;

public:
    class DefineParameterPosition
    {
    public:
        unsigned m_parameter_index;
        size_t m_parameter_position;
        bool m_stringize;

        DefineParameterPosition();
        DefineParameterPosition(unsigned index, size_t position, bool stringize);
    };

    class Define
    {
    public:
        std::string m_name;
        std::string m_value;
        std::vector<DefineParameterPosition> m_parameter_positions;
        bool m_contains_token_pasting_operators;

        Define();
        Define(std::string name, std::string value);
        void IdentifyParameters(const std::vector<std::string>& parameterNames);

    private:
        void IdentifyTokenPasteOperatorOnly();
    };

    enum class ParameterState : uint8_t
    {
        NOT_IN_PARAMETERS,
        AFTER_OPEN,
        AFTER_PARAM,
        AFTER_COMMA
    };

    class MacroParameterState
    {
    public:
        ParameterState m_parameter_state;
        std::ostringstream m_current_parameter;
        std::vector<std::string> m_parameters;
        std::stack<char> m_bracket_depth;

        MacroParameterState();
    };

private:
    enum class BlockMode : uint8_t

    {
        NOT_IN_BLOCK,
        IN_BLOCK,
        BLOCK_BLOCKED
    };

    IParserLineStream* const m_stream;
    const bool m_skip_directive_lines;
    std::map<std::string, Define> m_defines;
    std::stack<BlockMode> m_modes;
    unsigned m_ignore_depth;

    bool m_in_define;
    ParameterState m_parameter_state;
    Define m_current_define;
    std::ostringstream m_current_define_value;
    std::vector<std::string> m_current_define_parameters;

    const Define* m_current_macro;
    MacroParameterState m_multi_line_macro_parameters;

    static int GetLineEndEscapePos(const ParserLine& line);
    void MatchDefineParameters(const ParserLine& line, size_t& currentPos);
    void ContinueDefine(const ParserLine& line, size_t currentPos);
    void ContinueParameters(const ParserLine& line, size_t& currentPos);
    _NODISCARD bool MatchDefineDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchUndefDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchIfDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchElIfDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchIfdefDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchElseDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchEndifDirective(const ParserLine& line, size_t directiveStartPosition, size_t directiveEndPosition);
    _NODISCARD bool MatchDirectives(const ParserLine& line);

    static void ExtractParametersFromMacroUsage(
        const ParserLine& line, const unsigned& linePos, MacroParameterState& state, const std::string& input, unsigned& inputPos);
    bool FindMacroForIdentifier(const std::string& input, unsigned identifierStart, unsigned identifierEnd, const Define*& value) const;

    static bool MatchDefinedExpression(const ParserLine& line, size_t& pos, std::string& definitionName);
    void ExpandDefinedExpressions(ParserLine& line) const;

    bool FindNextMacro(const std::string& input, unsigned& inputPos, unsigned& defineStart, const DefinesStreamProxy::Define*& define) const;

    static void ProcessTokenPastingOperators(
        const ParserLine& line, const unsigned& linePos, std::vector<const Define*>& callstack, std::string& input, unsigned& inputPos);
    static void InsertMacroParameters(std::ostringstream& out, const DefinesStreamProxy::Define* macro, const std::vector<std::string>& parameterValues);
    void ExpandMacro(ParserLine& line,
                     unsigned& linePos,
                     std::ostringstream& out,
                     std::vector<const Define*>& callstack,
                     const DefinesStreamProxy::Define* macro,
                     const std::vector<std::string>& parameterValues);

    static void
        ContinueMacroParameters(const ParserLine& line, const unsigned& linePos, MacroParameterState& state, const std::string& input, unsigned& inputPos);
    void ContinueMultiLineMacro(ParserLine& line);

    void ProcessNestedMacros(ParserLine& line, unsigned& linePos, std::vector<const Define*>& callstack, std::string& input, unsigned& inputPos);
    void ProcessMacrosSingleLine(ParserLine& line);
    void ProcessMacrosMultiLine(ParserLine& line);

public:
    explicit DefinesStreamProxy(IParserLineStream* stream, bool skipDirectiveLines = false);

    void AddDefine(Define define);
    void Undefine(const std::string& name);

    _NODISCARD std::unique_ptr<ISimpleExpression> ParseExpression(std::shared_ptr<std::string> fileName, int lineNumber, std::string expressionString);

    ParserLine NextLine() override;
    bool IncludeFile(const std::string& filename) override;
    void PopCurrentFile() override;
    _NODISCARD bool IsOpen() const override;
    _NODISCARD bool Eof() const override;
};
