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
    mutable int m_permission_scope = 0;
    std::vector<CommonDDLStructDef> m_structs;
    std::vector<CommonDDLEnumDef> m_enums;
    mutable std::vector<CommonDDLMemberDef> m_member_stack;
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

    constexpr DDLGameFeatures GetFeatures() const
    {
        return DDL_GAME_FEATURES;
    };

    CommonDDLDef(const int version, const std::string& filename, CommonDDLRoot& root, const bool isInclude = false);

    void NameError(const std::string& message) const;
    const void ValidateName(bool isType, const std::string& name) const;
    bool Resolve();

    const bool Validate() const;
    bool Calculate();

private:
    const void ValidateRoot() const;
    void PreCalculate();
};