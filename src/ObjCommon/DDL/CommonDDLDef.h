#pragma once

class CommonDDLInclude;

class CommonDDLDef
{
public:
    const int m_version;
    const std::string m_filename;
    int m_size;
    std::unordered_map<DDLString, CommonDDLStructDef> m_structs;
    std::unordered_map<DDLString, CommonDDLEnumDef> m_enums;
    mutable std::vector<std::reference_wrapper<const CommonDDLMemberDef>> m_member_stack;
    mutable std::vector<int> m_in_calculation;

    void LogicError(const std::string& message) const;
    const size_t TypeToStructIndex(const DDLString& typeName) const noexcept;
    const int TypeToEnumIndex(const DDLString& typeName) const noexcept;
    const std::optional<DDLString> StructIndexToType(const size_t index) const noexcept;
    const std::optional<DDLString> EnumIndexToType(const int index) const noexcept;

    std::optional<CommonDDLStructDef&> GetStructByName(const DDLString& name, bool checkIncludes = false);
    std::optional<CommonDDLEnumDef&> GetEnumByName(const DDLString& name, bool checkIncludes = false);
    std::optional<CommonDDLStructDef&> GetStructByIndex(const size_t index);
    std::optional<CommonDDLEnumDef&> GetEnumByIndex(const int index);
    std::vector<CommonDDLInclude>& GetIncludes();
    void SetIncludes(std::vector<CommonDDLInclude>& includes);
    void AddStructFromInclude(CommonDDLStructDef& includeStruct);
    void AddEnumFromInclude(CommonDDLEnumDef& includeEnum);

    virtual const DDLGameFeatures& GetFeatures(){};
    virtual void Convert(const CommonDDLDef& from, void* to){};
    virtual void Convert(const void* from, CommonDDLDef& to){};

    CommonDDLDef(const int version, const std::string& filename);

    void NameError(const std::string& message) const;
    void ValidateName(const DDLString& name) const;
    void ResolveCustomTypes();

private:
    std::vector<CommonDDLInclude> m_includes;

    bool Validate() const;
    void ValidateRoot() const;
    void PreCalculate();
    bool Calculate();
};

class CommonDDLInclude : public CommonDDLDef
{
public:
    CommonDDLInclude(const int version, const std::string& filename);
};