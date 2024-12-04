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
    int m_size = 0;
    int m_offset = 0;
    size_t m_type_category = -1;
    ddlCategoryFlags_e m_category_flags = DDL_CATEGORY_FLAG_NONE;
    int m_external_index = 0;
    int m_enum_index = -1;

    [[noreturn]] void LogicError(const std::string& message) const;

    const bool IsStandardSize() const;
    const bool IsStandardType() const;
    const size_t GetStandardSize() const;
    const uint64_t GetStandardMaxValue() const;
    const int64_t GetStandardMinValue() const;
    const std::string PermissionTypeToName() const;
    const std::string TypeToName() const;
    const size_t NameToType() const;
    const int NameToPermissionType(const std::string& typeName) const;
    const bool IsUserDefinedType() const;
    const bool TypeCanUseBitfields() const;
    const bool TypeCanUseLimits() const;
    const bool TypeCanUseFixedFloatingPoint() const;
    const bool TypeIsSigned() const;
    const bool IsStringType() const;
    const size_t GetGameStructType() const;
    const size_t GetGameEnumType() const;
    const bool IsValidType() const;
    const bool IsValidPermission() const;

    const std::string GetCategoryTypeNames() const;
    void SetCategoryTypeFlags(const ddlCategoryFlags_e flags);
    void SetTypeInfo();
    const bool HasEnum() const;
    CommonDDLStructDef* GetParentStruct();
    const CommonDDLStructDef* GetParentStruct() const;
    void SetParentStruct(CommonDDLStructDef* parent);
    void ReportCircularDependency(const std::string message) const;
    void Validate() const;
    void Calculate();
    void ResetCalculated();
    void Resolve(bool referencedByRoot);

    CommonDDLMemberDef(const std::string name, CommonDDLStructDef* parent);
    CommonDDLStructDef* m_parent_struct;

private:
    size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    bool m_resolved = false;

    const void ValidateName() const;
    const void ValidateType() const;
    const void ValidatePermission() const;
    const void ValidateArray() const;
    const void ValidateMaxCharacters() const;
    const void ValidateEnum_() const;
    const void ValidateBitFields() const;
    const void ValidateRange() const;
    const void ValidateFixedPoint() const;
};