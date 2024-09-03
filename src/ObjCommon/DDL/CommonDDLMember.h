#pragma once

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
};

class CommonDDLMemberDefLinkData
{
public:
    int m_size = 0;
    int m_offset = 0;
    size_t m_type_enum = -1;
    ddlUserDefinedTypeFlags_e m_user_type_flags = DDL_USER_TYPE_NONE;
    int m_external_index = 0;
    int m_enum_index = -1;
    std::optional<DDLString> m_struct;
};

class CommonDDLMemberDef
{
public:
    DDLString m_name;
    DDLString m_type;
    std::optional<CommonDDLMemberLimits> m_limits;
    std::optional<int> m_array_size;
    std::optional<int> m_permission;
    std::optional<DDLString> m_enum;
    std::optional<int> m_max_characters;
    CommonDDLMemberDefLinkData m_link_data;

    void LogicError(const std::string& message) const;
    virtual const bool IsStandardSize() const {};
    virtual const bool IsStandardType() const {};
    virtual const size_t GetStandardSize() const {};
    virtual const size_t GetStandardMaxValue() const {};
    virtual const size_t GetStandardMinValue() const {};
    virtual const std::string& PermissionTypeToName() const {};
    virtual const std::string& TypeToName() const {};
    virtual size_t NameToType() const {};
    virtual int NameToPermissionType(const DDLString& typeName) const {};
    const bool IsUserDefinedType() const;
    virtual const bool TypeCanUseBitfields() const {};
    virtual const bool TypeCanUseLimits() const {};
    virtual const bool TypeCanUseFixedFloatingPoint() const {};
    virtual const bool IsStringType() const {};
    virtual const size_t GetGameStructType() const {};
    virtual const size_t GetGameEnumType() const {};
    virtual const bool IsValidType() const {};
    virtual const bool IsValidPermission() const {};
    const bool HasEnum() const;
    CommonDDLStructDef& GetParent();
    const CommonDDLStructDef& GetParentConst() const;
    void ReportCircularDependency(const std::string message) const;
    void Validate() const;
    void Calculate();
    void ResetCalculated();

    CommonDDLMemberDef(CommonDDLStructDef& parent);

private:
    mutable size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    CommonDDLStructDef& m_parent;

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