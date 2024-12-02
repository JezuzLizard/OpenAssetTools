#pragma once

#include "CommonDDL.h"

#include <iostream>
#include <cassert>

struct File
{
    std::string name;                            // The name of the file
    std::vector<std::shared_ptr<File>> includes; // The list of included files

    File(const std::string& name)
        : name(name)
    {
    }

    void AddInclude(const std::shared_ptr<File>& file)
    {
        includes.push_back(file);
    }

    void PrintIncludes(auto depth = 0u) const
    {
        std::string message = "";
        for (int i = 0u; i < depth; ++i)
        {
            message += " ";
        }

        std::cout << message + name << std::endl;
        for (const auto& include : includes)
        {
            include->PrintIncludes(depth + 1u);
        }
    }
};

class CommonDDLStructDef;
class CommonDDLEnumDef;
class CommonDDLMemberDef;
class CommonDDLRoot;

class CommonDDLDef
{
public:
    const int m_version;
    const std::string m_filename;
    const bool m_is_include;
    //const bool m_from_json;
    CommonDDLRoot& m_root;
    int m_size = 0;
    mutable size_t m_reference_count = 0;
    int m_permission_scope = 0;
    std::unordered_map<std::string, CommonDDLStructDef> m_structs;
    std::unordered_map<std::string, CommonDDLEnumDef> m_enums;
    mutable std::vector<std::reference_wrapper<const CommonDDLMemberDef>> m_member_stack;
    mutable std::vector<int> m_in_calculation;

    [[noreturn]] void LogicError(const std::string& message) const;
    const size_t TypeToStructIndex(const std::string& typeName) const noexcept;
    const int TypeToEnumIndex(const std::string& typeName) const noexcept;
    const std::optional<std::reference_wrapper<const std::string>> StructIndexToType(const size_t index) const noexcept;
    const std::optional<std::reference_wrapper<const std::string>> EnumIndexToType(const int index) const noexcept;

    CommonDDLStructDef* GetStructByName(const std::string& name, bool checkIncludes = false);
    CommonDDLEnumDef* GetEnumByName(const std::string& name, bool checkIncludes = false);
    CommonDDLStructDef* GetStructByIndex(const size_t index);
    CommonDDLEnumDef* GetEnumByIndex(const int index);

    void AddStructFromInclude(CommonDDLStructDef& includeStruct);
    void AddEnumFromInclude(CommonDDLEnumDef& includeEnum);
    CommonDDLRoot& GetRoot();
    const CommonDDLRoot& GetRoot() const;

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

    CommonDDLDef(const int version, const std::string& filename, CommonDDLRoot& root, const bool isInclude = false);

    void NameError(const std::string& message) const;
    void ValidateName(const std::string& name) const;
    bool Resolve();

    bool Validate() const;
    bool Calculate();

private:
    void ValidateRoot() const;
    void PreCalculate();
};