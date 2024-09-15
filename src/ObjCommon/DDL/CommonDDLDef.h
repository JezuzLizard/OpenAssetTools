#pragma once

#include "CommonDDL.h"

class CommonDDLStructDef;
class CommonDDLEnumDef;
class CommonDDLMemberDef;

class CommonDDLDef
{
public:
    const int m_version;
    const std::string m_filename;
    int m_size;
    std::unordered_map<std::string, CommonDDLStructDef> m_structs;
    std::unordered_map<std::string, CommonDDLEnumDef> m_enums;
    mutable std::vector<std::reference_wrapper<const CommonDDLMemberDef>> m_member_stack;
    mutable std::vector<int> m_in_calculation;

    void LogicError(const std::string& message) const;
    const size_t TypeToStructIndex(const std::string& typeName) const noexcept;
    const int TypeToEnumIndex(const std::string& typeName) const noexcept;
    const std::optional<std::string> StructIndexToType(const size_t index) const noexcept;
    const std::optional<std::string> EnumIndexToType(const int index) const noexcept;

    /*
    std::optional<CommonDDLStructDef> GetStructByName(const std::string& name, bool checkIncludes = false);
    std::optional<CommonDDLEnumDef> GetEnumByName(const std::string& name, bool checkIncludes = false);
    std::optional<CommonDDLStructDef> GetStructByIndex(const size_t index);
    std::optional<CommonDDLEnumDef> GetEnumByIndex(const int index);
    */
    void AddStructFromInclude(CommonDDLStructDef& includeStruct);
    void AddEnumFromInclude(CommonDDLEnumDef& includeEnum);

    virtual constexpr DDLGameFeatures GetFeatures() const
    {
        constexpr DDLGameFeatures DDL_GAME_FEATURES_DEFAULTS = {.m_simple_hash_table = false,
                                                                   .m_split_hash_table = false,
                                                                   .m_header = false,
                                                                   .m_padding = false,
                                                                   .m_uint_type = false,
                                                                   .m_fixed_point_type = false,
                                                                   .m_bool_type = false};

        return DDL_GAME_FEATURES_DEFAULTS;
    };

    CommonDDLDef(const int version, std::string filename);

    void NameError(const std::string& message) const;
    void ValidateName(const std::string& name) const;
    void ResolveCustomTypes();
    bool Validate() const;
    bool Calculate();

private:
    void ValidateRoot() const;
    void PreCalculate();
};