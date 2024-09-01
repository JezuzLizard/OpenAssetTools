#include "CommonDDL.h"

#include "CommonDDLDef.h"
#include "Utils/Alignment.h"

#include <cassert>

    CommonDDLMemberDef::Exception::Exception(std::string& message)
    : JsonDDLParseException::JsonDDLParseException(message)
{
}

CommonDDLStructDef& CommonDDLMemberDef::GetParent()
{
    return m_parent;
}

const CommonDDLStructDef& CommonDDLMemberDef::GetParentConst() const
{
    return m_parent;
}

void CommonDDLMemberDef::ResetCalculated()
{
    m_calculated = false;
}

const bool CommonDDLMemberDef::IsUserDefinedType() const
{
    return m_link_data.m_struct.has_value();
}

const bool CommonDDLMemberDef::HasEnum() const
{
    return m_enum.has_value();
}

void CommonDDLMemberDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL Member: {} Type: {} Parent: {}",
                                                m_name,
                                                m_link_data.m_struct.has_value()
                                                    ? m_link_data.m_struct.value()
                                                    : TypeToName(), GetParentConst()) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw CommonDDLMemberDef::Exception(prefaceAndMessage);
}

void CommonDDLMemberDef::ReportCircularDependency(std::string message) const
{
    const auto& parentDef = GetParentConst().GetParentConst();
    std::string traceback("Traceback:\n");
    for (auto i = 0u; i < parentDef.m_member_stack.size(); i++)
    {
        traceback += std::format("\tName: {}, Type: {} Parent: {}\n",
                                 parentDef.m_member_stack[i].m_name,
                                 parentDef.m_member_stack[i].m_link_data.m_struct.has_value() ? parentDef.m_member_stack[i].m_link_data.m_struct.value()
                                                                                          : parentDef.m_member_stack[i].TypeToName(),
                                 parentDef.m_member_stack[i].GetParentConst());
    }
    std::string prefaceAndMessage = std::format("{}\n{}", message, traceback);
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw CommonDDLMemberDef::Exception(prefaceAndMessage);
}

void CommonDDLMemberDef::Validate() const
{
    m_reference_count++;
    if (m_calculated)
        return;

    ValidateName();
    ValidateType();
    ValidatePermission();
    ValidateArray();
    ValidateEnum_();

    if (m_limits.has_value())
    {
        ValidateBitFields();
        ValidateRange();
        ValidateFixedPoint();
    }

    ValidateMaxCharacters();

    const auto& parentStruct = GetParentConst();
    const auto& parentDef = parentStruct.GetParentConst();
    if (m_enum.has_value())
    {
        auto index = parentDef.TypeToEnumIndex(m_enum.value());
        if (!index)
        {
            LogicError("could not create enumIndex for _enum field");
        }
        auto& enumDef = parentDef.m_enums.at(index);
        enumDef.Validate();
    }

    auto index = parentDef.TypeToStructIndex(m_type);
    if (index)
    {
        if (parentDef.m_in_calculation.at(index))
            ReportCircularDependency("circular dependency detected");

        const auto& structDef = parentDef.m_structs.at(index);
        parentDef.m_in_calculation[index] = true;
        parentDef.m_member_stack.push_back(*this);
        structDef.Validate();
        parentDef.m_member_stack.pop_back();
    }

    m_calculated = true;
    parentDef.m_in_calculation.clear();
    parentDef.m_in_calculation.resize(parentDef.m_structs.size(), false);
}

void CommonDDLMemberDef::Calculate()
{
    if (m_calculated)
        return;

    m_permission.emplace(GetParent().m_permission_scope);

    auto& parentDef = GetParent().GetParent();

    auto calculatedSize = 0;
    if (IsStandardType())
    {
        if (!m_limits.has_value())
            calculatedSize = GetStandardSize();
        else if (m_limits->m_bits.has_value())
            calculatedSize = m_limits->m_bits.value();
        else if (m_limits->m_range.has_value())
            calculatedSize = std::bit_width(m_limits->m_range.value());
        else if (m_limits->m_magnitude.has_value() && m_limits->m_precision.has_value())
        {
            m_limits->m_range.emplace(std::bit_width(m_limits->m_precision.value()));
            calculatedSize = m_limits->m_magnitude.value() + m_limits->m_precision.value();
        }
    }
    else if (IsStringType())
    {
        if (m_max_characters.has_value())
            calculatedSize = m_max_characters.value() * CHAR_BIT;
    }
    else if (IsUserDefinedType())
    {
        if (m_link_data.m_external_index)
        {
            if (parentDef.m_in_calculation.at(m_link_data.m_external_index))
                LogicError("circular dependency detected");

            auto& structDef = parentDef.m_structs.at(m_link_data.m_external_index);
            parentDef.m_in_calculation[m_link_data.m_external_index] = true;
            structDef.Calculate();
            calculatedSize = structDef.m_size;
        }
    }

    if (HasEnum())
    {
        calculatedSize = (calculatedSize * m_enum->size());
    }
    else
        calculatedSize = (calculatedSize * m_array_size.value_or(1));

    if (!calculatedSize)
        LogicError("failed to calculate size");

    m_link_data.m_size = calculatedSize;

    m_calculated = true;
    parentDef.m_in_calculation.clear();
    parentDef.m_in_calculation.resize(parentDef.m_structs.size(), false);
}

void CommonDDLMemberDef::ValidateName() const
{
    GetParentConst().GetParentConst().ValidateName(m_name.GetLowerConst());

    const auto& parentDef = GetParentConst().GetParentConst();
    auto nameRedefinitions = 0u;
    for (auto i = 0u; i < parentDef.m_structs.size(); i++)
    {
        if (parentDef.m_structs[i].m_name.GetLowerConst() == m_name.GetLowerConst())
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("member cannot have the same name as a struct");
    }

    for (auto i = 0u; i < parentDef.m_enums.size(); i++)
    {
        if (parentDef.m_enums[i].m_name.GetLowerConst() == m_name.GetLowerConst())
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("member cannot have the same name as an enum");
    }
}

void CommonDDLMemberDef::ValidateType() const
{
    if (!IsValidType())
        LogicError("unknown type");

    if ((m_link_data.m_user_type_flags & (DDL_USER_TYPE_ENUM|DDL_USER_TYPE_STRUCT)) == 0)
        return;

    const auto& parentDef = GetParentConst().GetParentConst();

    parentDef.ValidateName(m_type);

    for (auto i = 0u; i < parentDef.m_enums.size(); i++)
    {
        if (parentDef.m_enums[i].m_name.GetLowerConst() != m_type.GetLowerConst())
            continue;

        LogicError("type field cannot be set to an enum, the enum_ field is required to be set instead");
    }

    if ((m_link_data.m_user_type_flags & DDL_USER_TYPE_STRUCT) == 0)
        return;

    for (auto i = 0u; i < parentDef.m_structs.size(); i++)
    {
        if (parentDef.m_structs[i].m_name.GetLowerConst() == m_type.GetLowerConst())
            return;
    }

    LogicError("no definition found for type value");
}

void CommonDDLMemberDef::ValidatePermission() const
{
    if (GetParentConst().m_name != "root")
        LogicError("permission cannot be defined outside of root");
    if (!IsValidPermission())
        LogicError("permission must be client, server, both, or unspecified(defaults to both)");
}

void CommonDDLMemberDef::ValidateArray() const
{
    if (!m_array_size.has_value())
        return;
    if (m_enum.has_value())
        LogicError("arraySize field cannot be combined with enum_ field");
    if (m_array_size.value() <= 0)
        LogicError("arraySize cannot be 0");
}

void CommonDDLMemberDef::ValidateMaxCharacters() const
{
    if (IsStringType())
    {
        if (!m_max_characters.has_value())
            LogicError("string type requires maxCharacters field");
        if (m_max_characters.value() <= 0)
            LogicError("maxCharacters cannot be 0");

        return;
    }
    else if (!m_max_characters.has_value())
        return;

    LogicError("maxCharacters field cannot be used with types other than string");
}

void CommonDDLMemberDef::ValidateEnum_() const
{
    if (!HasEnum())
        return;

    assert((m_link_data.m_user_type_flags & DDL_USER_TYPE_ENUM) == 0);

    if (m_array_size.has_value())
        LogicError("arraySize field cannot be combined with enum_ field");

    const auto& parentDef = GetParentConst().GetParentConst();
    for (const auto& enum_ : parentDef.m_enums)
    {
        if (m_enum->GetLowerConst() != enum_.m_name.GetLowerConst())
            continue;

        return;
    }

    LogicError("could not find definition for enum");
}

void CommonDDLMemberDef::ValidateBitFields() const
{
    if (!m_limits->m_bits.has_value())
        return;

    if (!TypeCanUseBitfields())
        LogicError("type does not support bitfields");
    if (m_limits->m_range.has_value() || m_limits->m_precision.has_value() || m_limits->m_magnitude.has_value())
        LogicError("range, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with bits field");
    if (m_limits->m_bits.value() > GetStandardSize())
        LogicError("bits exceeds maximum possible bits for type");
}

void CommonDDLMemberDef::ValidateRange() const
{
    if (!m_limits->m_range.has_value())
        return;

    if (!TypeCanUseBitfields())
        LogicError("type does not support range field");
    if (m_limits->m_bits.has_value() || m_limits->m_precision.has_value() || m_limits->m_magnitude.has_value())
        LogicError("bits, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with range field");
    if (m_limits->m_range.value() > GetStandardMaxValue())
        LogicError("range exceeds maximum possible value for type");
}

void CommonDDLMemberDef::ValidateFixedPoint() const
{
    if (!m_limits->m_magnitude.has_value() && !m_limits->m_precision.has_value())
        if (TypeCanUseFixedFloatingPoint())
            LogicError("fixed_float requires both fixedMagnitudeBits, and fixedPrecisionBits fields");

    if (!TypeCanUseFixedFloatingPoint())
        LogicError("type must be fixed_float in order to use fixedMagnitudeBits, and FixedPrecisionBits");
    if (m_limits->m_range.has_value() || m_limits->m_bits.has_value())
        LogicError("range, and bits fields cannot be used with fixed_float type");
    if ((m_limits->m_magnitude.value() + m_limits->m_precision.value()) > OAT_DDL_FLOAT_BITS)
        LogicError(std::format("magnitude, and precision bits combined cannot exceed {} bits", OAT_DDL_FLOAT_BITS));
}
