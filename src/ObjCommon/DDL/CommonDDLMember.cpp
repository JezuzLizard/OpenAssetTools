#include "CommonDDL.h"

CommonDDLMemberDef::CommonDDLMemberDef(const std::string& name, CommonDDLStructDef& parent)
    : m_name(std::move(name)),
    m_parent(parent)
{
}

CommonDDLMemberLimits::CommonDDLMemberLimits(std::optional<size_t> bits,
                                           std::optional<size_t> range,
                                           std::optional<size_t> precision,
                                             std::optional<size_t> magnitude)
    : m_bits(bits),
      m_range(range),
      m_precision(precision),
      m_magnitude(magnitude)
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
                                                m_link_data.m_struct.has_value() ? m_link_data.m_struct.value().get().m_name
                                                    : TypeToName(), GetParentConst().m_name) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLMemberDef::ReportCircularDependency(std::string message) const
{
    const auto& parentDef = GetParentConst().GetParentConst();
    std::string traceback("\nTraceback:\n");
    for (const auto& member : parentDef.m_member_stack)
    {
        traceback += std::format("\t[Name:{} | Type:{} | Parent:{}]\n",
                                 member.get().m_name,
                                 member.get().m_link_data.m_struct.has_value() ? member.get().m_link_data.m_struct->get().m_name : member.get().TypeToName(),
                                 member.get().GetParentConst());
    }
    std::string prefaceAndMessage = std::format("{}\n{}", message, traceback);
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLMemberDef::Validate() const
{
    m_reference_count++;
    if (m_calculated)
        return;

    ValidateName();
    ValidateType();
    ValidatePermission();
    if (m_array_size.has_value())
        ValidateArray();
    if (!HasEnum())
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
    if (HasEnum())
    {
        auto& enumDef = parentDef.m_enums.at(m_name);
        enumDef.Validate();
    }

    auto index = parentDef.TypeToStructIndex(m_type);
    if (index)
    {
        if (parentDef.m_in_calculation.at(index))
            ReportCircularDependency("circular dependency detected");

        const auto& structDef = parentDef.m_structs.at(m_name);
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
            auto& structDef = parentDef.m_structs[m_name];
            parentDef.m_in_calculation[m_link_data.m_external_index] = true;
            structDef.Calculate();
            calculatedSize = structDef.m_size;
        }
    }

    if (HasEnum())
        calculatedSize = (calculatedSize * m_enum->size());
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
    GetParentConst().GetParentConst().ValidateName(m_name);

    const auto& parentDef = GetParentConst().GetParentConst();
    auto nameRedefinitions = 0u;
    for (const auto& [k, struc] : parentDef.m_structs)
    {
        if (struc.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("member cannot have the same name as a struct");
    }

    for (const auto& [k, enum_] : parentDef.m_enums)
    {
        if (enum_.m_name == m_name)
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

    if (parentDef.m_enums.find(m_type) != parentDef.m_enums.end())
        LogicError("type field cannot be set to an enum, the enum_ field is required to be set instead");

    if ((m_link_data.m_user_type_flags & DDL_USER_TYPE_STRUCT) == 0)
        return;

    if (parentDef.m_structs.find(m_type) != parentDef.m_structs.end())
        return;

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
    if (m_enum.has_value())
        LogicError("arraySize field cannot be combined with enum_ field");
    if (m_array_size.value() <= 0)
        LogicError("arraySize must be greater than 0");
}

void CommonDDLMemberDef::ValidateMaxCharacters() const
{
    if (IsStringType())
    {
        if (!m_max_characters.has_value())
            LogicError("string type requires maxCharacters field");
        if (m_max_characters.value() <= 0)
            LogicError("maxCharacters must be greater than 0");

        return;
    }
    else if (!m_max_characters.has_value())
        return;

    LogicError("maxCharacters field cannot be used with types other than string");
}

void CommonDDLMemberDef::ValidateEnum_() const
{
    assert((m_link_data.m_user_type_flags & DDL_USER_TYPE_ENUM) == 0);

    if (m_array_size.has_value())
        LogicError("arraySize field cannot be combined with enum_ field");

    const auto& parentDef = GetParentConst().GetParentConst();

    if (parentDef.m_enums.find(m_enum.value()) != parentDef.m_enums.end())
        return;

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

void CommonDDLMemberDef::Resolve()
{
    if (m_resolved)
        return;

    auto& parentDef = GetParent().GetParent();
    m_link_data.m_type_enum = NameToType();
    if (IsValidType())
    {
        if (m_link_data.m_type_enum == GetGameEnumType() || m_link_data.m_type_enum == GetGameStructType())
            LogicError("type cannot be directly defined as struct or enum");
    }

    auto typeStruct = parentDef.GetStructByName(m_type, true);
    if (!typeStruct.has_value())
        LogicError("could not resolve custom type for member");

    m_resolved = true;
    if (typeStruct->m_include_file.has_value())
    {
        parentDef.AddStructFromInclude(typeStruct.value());
    }

    if (!HasEnum())
        return;

    auto typeEnum = parentDef.GetEnumByName(m_enum.value(), true);
    if (!typeEnum.has_value())
        LogicError("could not resolve enum type for member");

    if (typeEnum->m_include_file.has_value())
    {
        parentDef.AddEnumFromInclude(typeEnum.value());
    }
}