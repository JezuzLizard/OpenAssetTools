#include "SequenceForwardDecl.h"

#include "Parsing/Header/Matcher/HeaderCommonMatchers.h"
#include "Parsing/Header/Matcher/HeaderMatcherFactory.h"

namespace
{
    static constexpr auto TAG_ENUM = 1;
    static constexpr auto TAG_STRUCT = 2;
    static constexpr auto TAG_UNION = 3;

    static constexpr auto CAPTURE_TYPE = 1;
    static constexpr auto CAPTURE_NAME = 2;
} // namespace

SequenceForwardDecl::SequenceForwardDecl()
{
    const HeaderMatcherFactory create(this);

    AddMatchers({
        create
            .Or({
                create.Type(HeaderParserValueType::ENUM).Tag(TAG_ENUM),
                create.Type(HeaderParserValueType::STRUCT).Tag(TAG_STRUCT),
                create.Type(HeaderParserValueType::UNION).Tag(TAG_UNION),
            })
            .Capture(CAPTURE_TYPE),
        create.Identifier().Capture(CAPTURE_NAME),
        create.Char(';'),
    });
}

void SequenceForwardDecl::ProcessMatch(HeaderParserState* state, SequenceResult<HeaderParserValue>& result) const
{
    const auto typeTag = result.NextTag();
    DataDefinitionType type;

    switch (typeTag)
    {
    case TAG_ENUM:
        type = DataDefinitionType::ENUM;
        break;

    case TAG_STRUCT:
        type = DataDefinitionType::STRUCT;
        break;

    case TAG_UNION:
        type = DataDefinitionType::UNION;
        break;

    default:
        throw ParsingException(result.NextCapture(CAPTURE_TYPE).GetPos(), "Unknown type");
    }

    state->AddForwardDeclaration(std::make_unique<ForwardDeclaration>(state->m_namespace.ToString(), result.NextCapture(CAPTURE_NAME).IdentifierValue(), type));
}
