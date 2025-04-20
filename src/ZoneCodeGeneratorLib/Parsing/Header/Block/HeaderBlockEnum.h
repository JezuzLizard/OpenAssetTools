#pragma once

#include "Domain/Definition/BaseTypeDefinition.h"
#include "Domain/Definition/EnumDefinition.h"
#include "IHeaderBlock.h"
#include "IHeaderBlockNameHolder.h"
#include "IHeaderBlockVariableDefining.h"
#include "Utils/ClassUtils.h"

#include <memory>
#include <string>
#include <vector>

class HeaderBlockEnum final : public IHeaderBlock, public IHeaderBlockNameHolder, public IHeaderBlockVariableDefining
{
public:
    HeaderBlockEnum(std::string typeName, const BaseTypeDefinition* parentType, bool isTypeDef);

    HeaderBlockType GetType() override;
    const std::vector<sequence_t*>& GetTestsForBlock() override;
    void OnOpen(HeaderParserState* state) override;
    void OnClose(HeaderParserState* state) override;
    void OnChildBlockClose(HeaderParserState* state, IHeaderBlock* block) override;

    void AddEnumMember(std::unique_ptr<EnumMember> enumMember);
    [[nodiscard]] EnumMember* GetEnumMember(const std::string& name) const;
    [[nodiscard]] int GetNextEnumMemberValue() const;

    void SetBlockName(const TokenPos& nameTokenPos, std::string name) override;
    bool IsDefiningVariable() override;
    DataDefinition* GetVariableType() override;
    std::string GetVariableName() override;

private:
    std::string m_namespace;
    std::string m_type_name;
    const BaseTypeDefinition* m_parent_type;
    bool m_is_typedef;
    std::vector<std::unique_ptr<EnumMember>> m_members;
    int m_next_value;
    EnumDefinition* m_enum_definition;

    std::string m_variable_name;
};
