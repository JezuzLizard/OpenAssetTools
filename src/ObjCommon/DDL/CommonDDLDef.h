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
class CommonDDLDef;

class CommonDDLInclude
{
public:
    std::string m_include_name;
    const CommonDDLDef& m_def;

    CommonDDLInclude(const std::string& name, const CommonDDLDef& def)
        : m_include_name(name),
          m_def(def)
    {
    }
};

class CommonDDLDef
{
public:
    const int m_version;
    const std::string m_filename;
    int m_size;
    std::unordered_map<std::string, CommonDDLStructDef> m_structs;
    std::unordered_map<std::string, CommonDDLEnumDef> m_enums;
    std::unordered_map<std::string, std::shared_ptr<CommonDDLInclude>> m_include_pool;
    mutable std::vector<std::reference_wrapper<const CommonDDLMemberDef>> m_member_stack;
    mutable std::vector<int> m_in_calculation;

    [[noreturn]] void LogicError(const std::string& message) const;
    const size_t TypeToStructIndex(const std::string& typeName) const noexcept;
    const int TypeToEnumIndex(const std::string& typeName) const noexcept;
    const std::optional<std::reference_wrapper<const std::string>> StructIndexToType(const size_t index) const noexcept;
    const std::optional<std::reference_wrapper<const std::string>> EnumIndexToType(const int index) const noexcept;

    std::optional<CommonDDLStructDef&> GetStructByName(const std::string& name, bool checkIncludes = false);
    std::optional<CommonDDLEnumDef&> GetEnumByName(const std::string& name, bool checkIncludes = false);
    std::optional<CommonDDLStructDef&> GetStructByIndex(const size_t index);
    std::optional<CommonDDLEnumDef&> GetEnumByIndex(const int index);

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

    CommonDDLDef(const int version, std::string& filename);

    void NameError(const std::string& message) const;
    void ValidateName(const std::string& name) const;
    void ResolveCustomTypes();

    void CreateInclude(const std::string& filename, const CommonDDLDef& def)
    {
        assert(m_version == def.m_version);
        if (m_include_pool.find(filename) == m_include_pool.end())
        {
            m_include_pool[filename] = std::make_shared<CommonDDLInclude>(filename, def);
        }
    };

    bool Validate() const;
    bool Calculate();

private:
    void ValidateRoot() const;
    void PreCalculate();
};