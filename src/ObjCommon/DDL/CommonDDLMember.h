#pragma once

#include "CommonDDL.h"

class CommonDDLStructDef;

class CommonDDLMemberLimits
{
public:
    // Represents the size of the member in bits.
    // Used by the bitfield syntax to explicitly define the size.
    std::optional<size_t> m_bits;
    // Represents the size of the member based on the magnitude of the value of .range.
    std::optional<size_t> m_range;
    // Represents the size of the member split between precision bits, and magnitude bits.
    std::optional<size_t> m_precision;
    // Represents the size of the member split between precision bits, and magnitude bits.
    std::optional<size_t> m_magnitude;

    virtual void CalculateFixedPoint(){};
    virtual void CalculateRangeLimits(){};
    virtual void CalculateBitLimits(){};

    CommonDDLMemberLimits(std::optional<size_t> bits,
                          std::optional<size_t> range,
                          std::optional<size_t> precision = std::nullopt,
                          std::optional<size_t> magnitude = std::nullopt);
};

class CommonDDLMemberDefLinkData
{
public:
    int m_size = 0;
    int m_offset = 0;
    size_t m_type_category = -1;
    ddlCategoryFlags_e m_category_flags = DDL_CATEGORY_FLAG_NONE;
    int m_external_index = 0;
    int m_enum_index = -1;
    std::string m_parent_struct = "";
};

class CommonDDLMemberDef
{
public:
    friend class CommonDDLDef;
    const std::string m_name;
    std::string m_type = "";
    std::optional<CommonDDLMemberLimits> m_limits;
    std::optional<int> m_array_size; // default 1
    std::optional<int> m_permission; // default both
    std::optional<std::string> m_enum;
    std::optional<int> m_max_characters;
    CommonDDLMemberDefLinkData m_link_data;

    [[noreturn]] void LogicError(const std::string& message) const;

    virtual const bool IsStandardSize() const
    {
        return 0;
    };

    virtual const bool IsStandardType() const
    {
        return 0;
    };

    virtual const size_t GetStandardSize() const
    {
        return 0;
    };

    virtual const uint64_t GetStandardMaxValue() const
    {
        return 0;
    };

    virtual const int64_t GetStandardMinValue() const
    {
        return 0;
    };

    virtual const std::string PermissionTypeToName() const
    {
        return "none";
    };

    virtual const std::string TypeToName() const
    {
        return "none";
    };

    virtual const size_t NameToType() const
    {
        return 0;
    };

    virtual const int NameToPermissionType(const std::string& typeName) const
    {
        return 0;
    };

    const bool IsUserDefinedType() const
    {
        return 0;
    };

    virtual const bool TypeCanUseBitfields() const
    {
        return 0;
    };

    virtual const bool TypeCanUseLimits() const
    {
        return 0;
    };

    virtual const bool TypeCanUseFixedFloatingPoint() const
    {
        return 0;
    };

    virtual const bool IsStringType() const
    {
        return 0;
    };

    virtual const size_t GetGameStructType() const
    {
        return 0;
    };

    virtual const size_t GetGameEnumType() const
    {
        return 0;
    };

    virtual const bool IsValidType() const
    {
        return 0;
    };

    virtual const bool IsValidPermission() const
    {
        return 0;
    };

    const std::string GetCategoryTypeNames() const;
    void SetCategoryTypeFlags(const ddlCategoryFlags_e flags);
    void SetTypeInfo();
    const bool HasEnum() const;
    CommonDDLStructDef& GetParentStruct();
    const CommonDDLStructDef& GetParentStruct() const;
    void ReportCircularDependency(const std::string message) const;
    void Validate() const;
    void Calculate();
    void ResetCalculated();
    void Resolve();

    CommonDDLMemberDef(const std::string name, CommonDDLStructDef& parent);

private:
    size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    bool m_resolved = false;
    CommonDDLStructDef& m_parent_struct;

    void ValidateName() const;
    void ValidateType() const;
    void ValidatePermission() const;
    void ValidateArray() const;
    void ValidateMaxCharacters() const;
    void ValidateEnum_() const;
    void ValidateBitFields() const;
    void ValidateRange() const;
    void ValidateFixedPoint() const;
};