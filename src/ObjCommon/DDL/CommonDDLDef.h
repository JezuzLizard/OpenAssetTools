#pragma once

class CommonDDLInclude;

class CommonDDLDef
{
public:
    int m_version;
    int m_size;
    std::vector<CommonDDLStructDef> m_structs;
    std::vector<CommonDDLEnumDef> m_enums;
    mutable std::vector<const CommonDDLMemberDef&> m_member_stack;
    mutable std::vector<bool> m_in_calculation;

    std::string m_filename;

    void LogicError(const std::string& message) const;
    size_t TypeToStructIndex(const DDLString& typeName) const noexcept;
    int TypeToEnumIndex(const DDLString& typeName) const noexcept;
    std::optional<DDLString> StructIndexToType(const size_t index) const noexcept;
    std::optional<DDLString> EnumIndexToType(const int index) const noexcept;

    std::optional<CommonDDLStructDef&> FindStruct(const DDLString& name, bool checkIncludes = false);
    std::optional<CommonDDLEnumDef&> FindEnum(const DDLString& name, bool checkIncludes = false);
    std::vector<CommonDDLInclude>& GetIncludes();

    virtual const DDLGameFeatures& GetFeatures(){};

    CommonDDLDef(const int version, const std::string filename);

    void NameError(const std::string& message) const;
    void ValidateName(const DDLString& name) const;

private:
    std::vector<CommonDDLInclude> m_includes;

    bool Validate() const;
    void ValidateRoot() const;
    void PreCalculate();
    bool Calculate();
};

class CommonDDLInclude : public CommonDDLDef
{

private:
    bool Validate() const;
};