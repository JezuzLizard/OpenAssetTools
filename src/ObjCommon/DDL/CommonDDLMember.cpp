#include "CommonDDL.h"
#include "CommonDDLDef.h"
#include "CommonDDLStruct.h"
#include "CommonDDLEnum.h"
#include "CommonDDLMember.h"

#include <format>
#include <cassert>

CommonDDLMemberDef::CommonDDLMemberDef(const std::string name,
                                       CommonDDLStructDef* parent)
    : m_name(std::move(name)),
      m_parent_struct(parent)
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

const std::string CommonDDLMemberDef::GetCategoryTypeNames() const
{
    return "";
}

void CommonDDLMemberDef::SetCategoryTypeFlags(const ddlCategoryFlags_e flags)
{
    assert(flags <= DDL_CATEGORY_FLAG_BOTH);
}

void SetTypeInfo()
{

}

CommonDDLStructDef* CommonDDLMemberDef::GetParentStruct()
{
    return m_parent_struct;
}

const CommonDDLStructDef* CommonDDLMemberDef::GetParentStruct() const
{
    return m_parent_struct;
}

void CommonDDLMemberDef::SetParentStruct(CommonDDLStructDef* parent)
{
    m_parent_struct = parent;
}

void CommonDDLMemberDef::ResetCalculated()
{
    m_calculated = false;
}

const bool CommonDDLMemberDef::HasEnum() const
{
    return m_enum.has_value();
}

[[noreturn]] void CommonDDLMemberDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL MEMBER ERROR: [Name: {}|Type: {}|Parent: {}] ",
                                                m_name,
                                                TypeToName(), 
                                                GetParentStruct()->m_name) + message;
#ifdef DDL_DEBUG
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

const bool CommonDDLMemberDef::IsStandardSize() const
{
    if (m_type_category > DDL_FLOAT_TYPE)
        return false;

    return DDL_TYPE_FEATURES[m_type_category].size == m_size / m_array_size.value_or(1);
};

const bool CommonDDLMemberDef::IsStandardType() const
{
    return m_type_category <= DDL_FLOAT_TYPE;
};

const size_t CommonDDLMemberDef::GetStandardSize() const
{
    return DDL_TYPE_FEATURES[m_type_category].size;
};

const uint64_t CommonDDLMemberDef::GetStandardMaxValue() const
{
    return DDL_TYPE_FEATURES[m_type_category].max;
};

const int64_t CommonDDLMemberDef::GetStandardMinValue() const
{
    return DDL_TYPE_FEATURES[m_type_category].min;
};

const std::string CommonDDLMemberDef::PermissionTypeToName() const
{
    if (!m_permission.has_value())
        return "unknown";

    for (auto& [k, v] : DDL_PERM_NAMES)
    {
        if (v == m_permission.value())
            return k;
    }

    return "unknown";
};

const std::string CommonDDLMemberDef::TypeToName() const
{
    if (!m_type.empty())
        return m_type;

    if (m_type_category > DDL_TYPE_NAMES.size())
        return "unknown";

    for (const auto& [k, v] : DDL_TYPE_NAMES)
    {
        if (v == m_type_category)
            return k;
    }

    return "unknown";
};

const size_t CommonDDLMemberDef::NameToType() const
{
    auto it = DDL_TYPE_NAMES.find(m_type);
    if (it != DDL_TYPE_NAMES.end())
        return static_cast<ddlPrimitiveTypes_e>(it->second);

    return DDL_TYPE_COUNT;
};

const int CommonDDLMemberDef::NameToPermissionType(const std::string& typeName) const
{
    auto it = DDL_PERM_NAMES.find(typeName);
    if (it != DDL_PERM_NAMES.end())
        return static_cast<ddlPermissionTypes_e>(it->second);

    return DDL_PERMISSIONS_COUNT;
};

const bool CommonDDLMemberDef::IsUserDefinedType() const
{
    return m_type_category == DDL_STRUCT_TYPE;
};

const bool CommonDDLMemberDef::TypeCanUseBitfields() const
{
    if (m_type_category >= DDL_STRING_TYPE)
        return false;
    return (DDL_TYPE_FEATURES[m_type_category].flags & DDL_FLAG_BITFIELDS) != 0;
};

const bool CommonDDLMemberDef::TypeCanUseLimits() const
{
    if (m_type_category >= DDL_STRING_TYPE)
        return false;
    return (DDL_TYPE_FEATURES[m_type_category].flags & DDL_FLAG_LIMITS) != 0;
};

const bool CommonDDLMemberDef::TypeCanUseFixedFloatingPoint() const
{
    return m_type_category == DDL_FIXEDPOINT_TYPE;
};

const bool CommonDDLMemberDef::IsStringType() const
{
    return m_type_category == DDL_STRING_TYPE;
};

const size_t CommonDDLMemberDef::GetGameStructType() const
{
    return DDL_STRUCT_TYPE;
};

const size_t CommonDDLMemberDef::GetGameEnumType() const
{
    return DDL_ENUM_TYPE;
};

const bool CommonDDLMemberDef::IsValidType() const
{
    return m_type_category < DDL_TYPE_COUNT;
};

const bool CommonDDLMemberDef::IsValidPermission() const
{
    return m_permission.value_or(DDL_PERMISSIONS_COUNT) < DDL_PERMISSIONS_COUNT;
};

void CommonDDLMemberDef::ReportCircularDependency(std::string message) const
{
    const auto& parentDef = GetParentStruct()->GetParentDef();
    std::string traceback("\nTraceback:\n");
    for (const auto& member : parentDef->m_member_stack)
    {
        traceback += std::format("\t[Name:{} | Type:{} | Parent:{}]\n",
                                 member.m_name,
                                 member.TypeToName(), member.GetParentStruct()->m_name);
    }
    std::string prefaceAndMessage = std::format("{}\n{}", message, traceback);
#ifdef DDL_DEBUG
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLMemberDef::Validate() const
{
    if (m_calculated)
        return;

    ValidateName();
    ValidateType();
    ValidatePermission();
    if (m_array_size.has_value())
        ValidateArray();
    if (HasEnum())
        ValidateEnum_();

    if (m_limits.has_value())
    {
        ValidateBitFields();
        ValidateRange();
        ValidateFixedPoint();
    }

    ValidateMaxCharacters();

    const auto* parentStruct = GetParentStruct();
    const auto* parentDef = parentStruct->GetParentDef();
    if (HasEnum())
    {
        const auto& enumDef = parentDef->m_enums[m_enum_index];
        enumDef.Validate();
    }

    auto index = m_external_index;
    if (index)
    {
        if (parentDef->m_in_calculation.at(index))
            ReportCircularDependency("circular dependency detected");

        parentDef->m_in_calculation[index] = true;
        parentDef->m_member_stack.push_back(*this);
        parentDef->m_structs[index].Validate();
        parentDef->m_member_stack.pop_back();
    }

    m_calculated = true;
    parentDef->m_in_calculation.clear();
    parentDef->m_in_calculation.resize(parentDef->m_structs.size(), false);
}

void CommonDDLMemberDef::Calculate()
{
    if (m_calculated)
        return;

    auto* parentDef = GetParentStruct()->GetParentDef();

    auto calculatedSize = 0;
    if (IsStandardType())
    {
        if (!m_limits.has_value())
            calculatedSize = GetStandardSize();
        else if (m_limits->m_bits.has_value())
            calculatedSize = m_limits->m_bits.value();
        else if (m_limits->m_range.has_value())
            calculatedSize = std::bit_width(m_limits->m_range.value());
    }
    else if (TypeCanUseFixedFloatingPoint())
    {
        m_limits->m_range.emplace(std::bit_width(m_limits->m_precision.value()));
        calculatedSize = m_limits->m_magnitude.value() + m_limits->m_precision.value();
    }
    else if (IsStringType())
    {
        if (m_max_characters.has_value())
            calculatedSize = m_max_characters.value() * CHAR_BIT;
    }
    else if (IsUserDefinedType())
    {
        if (m_external_index)
        {
            auto& structDef = parentDef->m_structs[m_external_index];
            parentDef->m_in_calculation[m_external_index] = true;
            structDef.Calculate();
            calculatedSize = structDef.m_size;
        }
    }

    if (HasEnum())
        calculatedSize = (calculatedSize * m_enum->size());
    else
        calculatedSize = (calculatedSize * m_array_size.value_or(1));

    if (!calculatedSize)
        LogicError("failed to calculate size, unhandled type");

    m_size = calculatedSize;

    m_calculated = true;
    parentDef->m_in_calculation.clear();
    parentDef->m_in_calculation.resize(parentDef->m_structs.size(), false);
}

const void CommonDDLMemberDef::ValidateName() const
{
    const auto* parentDef = GetParentStruct()->GetParentDef();
    parentDef->ValidateName(false, m_name);
    auto nameRedefinitions = 0u;
    for (const auto& struc : parentDef->m_structs)
    {
        if (struc.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("member cannot have the same name as a struct");
    }

    for (const auto& enum_ : parentDef->m_enums)
    {
        if (enum_.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("member cannot have the same name as an enum");
    }
}

const void CommonDDLMemberDef::ValidateType() const
{
    if (!IsValidType())
        LogicError("unknown type");

    if ((m_category_flags & (DDL_CATEGORY_FLAG_ENUM|DDL_CATEGORY_FLAG_STRUCT)) == 0)
        return;

    const auto* parentDef = GetParentStruct()->GetParentDef();

    parentDef->ValidateName(true, m_type);

    const auto& type = m_type;
    auto it = std::find_if(parentDef->m_enums.begin(),
                           parentDef->m_enums.end(),
                    [type](const CommonDDLEnumDef& _enum)
                    {
                         return type == _enum.m_name;
                    });
    if (it != parentDef->m_enums.end())
        LogicError("type field cannot be set to an enum, the enum_ field is required to be set instead");

    if ((m_category_flags & DDL_CATEGORY_FLAG_STRUCT) == 0)
        return;

    auto it2 = std::find_if(parentDef->m_structs.begin(),
                            parentDef->m_structs.end(),
                           [type](const CommonDDLStructDef& struc)
                           {
                               return type == struc.m_name;
                           });
    if (it2 != parentDef->m_structs.end())
        return;

    LogicError("no definition found for type value");
}

const void CommonDDLMemberDef::ValidatePermission() const
{
    if (!m_permission.has_value() || m_reference_count == 0)
        return;
    if (GetParentStruct()->m_name != "root")
        LogicError("permission cannot be defined outside of root");
    if (!IsValidPermission())
        LogicError("permission must be client, server, both, or unspecified(defaults to both)");
}

const void CommonDDLMemberDef::ValidateArray() const
{
    if (m_enum.has_value())
        LogicError("arraySize field cannot be combined with enum_ field");
    if (m_array_size.value() <= 0)
        LogicError("arraySize must be greater than 0");
}

const void CommonDDLMemberDef::ValidateMaxCharacters() const
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

const void CommonDDLMemberDef::ValidateEnum_() const
{
    assert((m_category_flags & DDL_CATEGORY_FLAG_ENUM) != 0);

    if (m_array_size.has_value())
        LogicError("arraySize field cannot be combined with enum_ field");

    const auto& parentDef = GetParentStruct()->GetParentDef();

    const auto& referencedEnum = m_enum.value();
    auto it = std::find_if(parentDef->m_enums.begin(),
                           parentDef->m_enums.end(),
                           [referencedEnum](const CommonDDLEnumDef& _enum)
                           {
                               return referencedEnum == _enum.m_name;
                           });
    if (it != parentDef->m_enums.end())
        return;

    LogicError("could not find definition for enum");
}

const void CommonDDLMemberDef::ValidateBitFields() const
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

const void CommonDDLMemberDef::ValidateRange() const
{
    if (!m_limits->m_range.has_value())
        return;

    if (!TypeCanUseLimits())
        LogicError("type does not support range field");
    if (m_limits->m_bits.has_value() || m_limits->m_precision.has_value() || m_limits->m_magnitude.has_value())
        LogicError("bits, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with range field");
    if (m_limits->m_range.value() > GetStandardMaxValue())
        LogicError("range exceeds maximum possible value for type");
}

const void CommonDDLMemberDef::ValidateFixedPoint() const
{
    if (!m_limits->m_magnitude.has_value() || !m_limits->m_precision.has_value())
        if (TypeCanUseFixedFloatingPoint())
            LogicError("fixed_float requires both fixedMagnitudeBits, and fixedPrecisionBits fields");
        else
            return;

    if (!TypeCanUseFixedFloatingPoint())
        LogicError("type must be fixed_float in order to use fixedMagnitudeBits, and FixedPrecisionBits");
    if (m_limits->m_range.has_value() || m_limits->m_bits.has_value())
        LogicError("range, and bits fields cannot be used with fixed_float type");
    if ((m_limits->m_magnitude.value() + m_limits->m_precision.value()) > OAT_DDL_FLOAT_BITS)
        LogicError(std::format("magnitude, and precision bits combined cannot exceed {} bits", OAT_DDL_FLOAT_BITS));
}

void CommonDDLMemberDef::Resolve(bool referencedByRoot)
{
    if (referencedByRoot)
        m_reference_count++;
    if (m_resolved)
        return;

    auto* parentDef = GetParentStruct()->GetParentDef();
    m_type_category = NameToType();

    if (m_type_category == GetGameEnumType() || m_type_category == GetGameStructType())
        LogicError("type cannot be directly defined as struct or enum");

    if (referencedByRoot)
    {
        assert(m_permission.has_value());
        m_permission.emplace(parentDef->m_permission_scope);
    }
    else
    {
        m_permission.emplace(DDL_PERMISSIONS_BOTH);
    }

    if (HasEnum())
    {
        auto* enum_ = parentDef->GetEnumByName(m_enum.value(), true);
        if (!enum_)
            LogicError("could not resolve enum for member");

        if (referencedByRoot)
        {
            enum_->GetParentDef()->m_reference_count++;
            enum_->m_reference_count++;
            if (enum_->GetParentDef()->m_reference_count <= 1 && enum_->m_from_include)
            {
                parentDef->AddEnumFromInclude(*enum_);
            }
        }

        m_type_category = NameToType();
        m_enum_index = enum_->m_index;
        m_category_flags = static_cast<ddlCategoryFlags_e>(m_category_flags | DDL_CATEGORY_FLAG_ENUM);
    }

    if (IsValidType())
    {
        // not a user defined type
        m_resolved = true;
        return;
    }

    auto* struc = parentDef->GetStructByName(m_type, true);
    if (!struc)
        LogicError("could not resolve custom struct type for member");

    m_resolved = true;
    if (referencedByRoot)
    {
        struc->GetParentDef()->m_reference_count++;
        struc->m_reference_count++;
        if (struc->GetParentDef()->m_reference_count <= 1 && struc->m_from_include)
        {
            parentDef->AddStructFromInclude(*struc);
        }
    }

    // the int type on the canonical ddlDef_t struct can only be one value, however enum, and struct are separate values
    // which one has priority?
    m_type_category = GetGameStructType(); 
    m_external_index = struc->m_index;
    m_category_flags = static_cast<ddlCategoryFlags_e>(m_category_flags | DDL_CATEGORY_FLAG_STRUCT);
}