
#include "CommonDDL.h"
#include "CommonDDLDef.h"
#include "CommonDDLStruct.h"
#include "CommonDDLEnum.h"
#include "CommonDDLMember.h"
#include "CommonDDLRoot.h"

#include <cassert>
#include <format>
#include <iostream>

CommonDDLDef::CommonDDLDef(const int version, const std::string& filename, CommonDDLRoot& root, const bool isInclude)
    : m_version(version),
      m_filename(std::move(filename)),
      m_root(root),
      m_is_include(isInclude)
{
}

CommonDDLRoot& CommonDDLDef::GetRoot()
{
    return m_root;
}

const CommonDDLRoot& CommonDDLDef::GetRoot() const
{
    return m_root;
}

CommonDDLStructDef* CommonDDLDef::GetStructByName(const std::string& structName, bool checkIncludes)
{
    if (m_structs.find(structName) != m_structs.end())
        return &m_structs[structName];

    if (checkIncludes)
    {
        assert(GetRoot().m_include_pool.contains(m_filename));

        for (auto& include : GetRoot().m_include_pool[m_filename])
        {
            assert(include.m_is_include);

            if (include.m_structs.find(structName) != include.m_structs.end())
                return &include.m_structs[structName];
        }
    }
    return nullptr;
}

CommonDDLEnumDef* CommonDDLDef::GetEnumByName(const std::string& enumName, bool checkIncludes)
{
    if (m_enums.find(enumName) != m_enums.end())
        return &m_enums[enumName];

    if (checkIncludes)
    {
        assert(GetRoot().m_include_pool.contains(m_filename));

        for (auto& include : GetRoot().m_include_pool[m_filename])
        {
            assert(include.m_is_include);

            if (include.m_enums.find(enumName) != include.m_enums.end())
                return &include.m_enums[enumName];
        }
    }

    return nullptr;
}

CommonDDLStructDef* CommonDDLDef::GetStructByIndex(const size_t index)
{
    if (index < m_structs.size())
    {
        /*
        auto* struc = std::ranges::find_if(m_structs,
                     [index](const CommonDDLStructDef& struc)
                     {
                           return struc.m_index == index;
                     });
        */
    }

    return nullptr;
}

CommonDDLEnumDef* CommonDDLDef::GetEnumByIndex(const int index)
{
    if (index >= 0 && index < m_enums.size())
    {
    }

    return nullptr;
}

[[noreturn]] void CommonDDLDef::LogicError(const std::string& message) const
{
    assert(!m_filename.empty());

    std::string prefaceAndMessage = std::format("DDL Def: {} ", m_filename) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

const size_t CommonDDLDef::TypeToStructIndex(const std::string& typeName) const noexcept
{
    auto i = 0u;
    for (const auto& [k, struc] : m_structs)
    {
        if (k == typeName)
            return i;
        i++;
    }

    return 0u;
}

const int CommonDDLDef::TypeToEnumIndex(const std::string& typeName) const noexcept
{
    auto i = 0u;
    for (const auto& [k, enum_] : m_enums)
    {
        if (k == typeName)
            return i;
        i++;
    }

    return -1;
}

const std::optional<std::reference_wrapper<const std::string>> CommonDDLDef::StructIndexToType(const size_t index) const noexcept
{
    auto i = 0u;
    for (const auto& [k, struc] : m_structs)
    {
        if (i == index)
            return std::optional<std::reference_wrapper<const std::string>>(struc.m_name);
        i++;
    }

    return std::nullopt;
}

const std::optional<std::reference_wrapper<const std::string>> CommonDDLDef::EnumIndexToType(const int index) const noexcept
{
    auto i = 0u;
    for (const auto& [k, enum_] : this->m_enums)
    {
        if (i == index)
        {
            return enum_.m_name;
        }
        i++;
    }

    return std::nullopt;
}

bool CommonDDLDef::Validate() const
{
    try
    {
        ValidateRoot();
        return true;
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}

void CommonDDLDef::PreCalculate()
{
    for (auto& [k, struc] : m_structs)
    {
        struc.ResetCalculated();
        for (auto& [k2, member] : struc.m_members)
            member.ResetCalculated();
    }
    for (auto& [k, enum_] : m_enums)
        enum_.ResetCalculated();

    m_in_calculation.clear();
    m_in_calculation.resize(m_structs.size());
}

bool CommonDDLDef::Calculate()
{
    PreCalculate();

    try
    {
        for (auto& [k, struc] : m_structs)
        {
            if (k == "root")
            {
                struc.Calculate();
                break;
            }
        }

        for (auto& [k, struc] : m_structs)
            // Structs can canonically be linked into the asset, but do not change the size of the ddl buffer as they are not in the root struct.
            // Treyarch likely had an include system that would paste structs from an external file into the the defs that include.
            // Or they had an export/import system.
            if (struc.GetRefCount())
                m_size += struc.m_size;

        // Link unreferenced structs anyway
        for (auto& [k, struc] : m_structs)
            if (!struc.GetRefCount())
                struc.Calculate();

        // Link unreferenced enums anyway
        for (auto& [k, enum_] : m_enums)
            if (!enum_.GetRefCount())
                enum_.CalculateHashes();

        for (auto& [k, enum_] : m_enums)
            enum_.CalculateHashes();

        return true;
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}

void CommonDDLDef::ValidateRoot() const
{
    for (const auto& [k, struc] : m_structs)
    {
        if (k == "root")
        {
            struc.Validate();
            break;
        }

        LogicError("root struct entry point missing");
    }

    // Check reference counts for enums, and structs to determine unused enums/structs.
    // If a struct is unused it will still be linked into the asset, but it won't contribute to the size of the def.
    // Track, and validate these unused enums/structs so that the user can be alerted to a mistake(they wanted to use the struct they made, but forgot to
    // actually reference it in root). Treyarch canonically allowed defs to include structs/enums from common files, the evidence is that there are multiple
    // instances of the same struct appearing in multiple defs, with instances of it not having any references occuring multiple times.

    for (const auto& [k, enum_] : m_enums)
    {
        enum_.ReferenceCount();
        if (!enum_.GetRefCount())
            enum_.Validate();
    }

    for (const auto& [k, struc] : m_structs)
    {
        struc.ReferenceCount();
        if (!struc.GetRefCount())
            struc.Validate();
    }
}

void CommonDDLDef::NameError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("File: {} ", this->m_filename) + message;
#ifdef DDL_DEBUG
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLDef::ValidateName(const std::string& name) const
{
    if (name.empty())
        NameError("field cannot be empty");

    if (!std::isalpha(name[0]))
        NameError(std::format("field cannot begin with \"{}\"", name[0]));

    for (auto i = 1u; i < name.length(); i++)
    {
        if (!std::isalnum(name[i]) && name.substr(i, 1) != "_")
            NameError(std::format("field cannot contain \"{}\"", name[i]));
    }

    const auto& it = DDL_KEYWORDS.find(name);
    if (it != DDL_KEYWORDS.end())
        NameError(std::format("field cannot be reserved keyword \"{}\"", it->data()));
}

void CommonDDLDef::AddStructFromInclude(CommonDDLStructDef& includeStruct)
{
    if (m_structs.find(includeStruct.m_name) == m_structs.end())
    {
        includeStruct.m_reference_count++;
        if (includeStruct.m_resolved)
            return;

        includeStruct.m_resolved = true;
        m_structs.emplace(includeStruct.m_name, includeStruct);
        includeStruct.Resolve();
    }
    else
        assert(false);
}

void CommonDDLDef::AddEnumFromInclude(CommonDDLEnumDef& includeEnum)
{
    if (m_enums.find(includeEnum.m_name) == m_enums.end())
    {
        includeEnum.m_reference_count++;
        if (includeEnum.m_resolved)
            return;

        includeEnum.m_resolved = true;
        m_enums.emplace(includeEnum.m_name, includeEnum);
    }
    else
        assert(false);
}

bool CommonDDLDef::Resolve()
{
    try
    {
        // Determine what structs from what includes are referenced
        for (auto& [k, struc] : m_structs)
        {
            if (struc.m_name != "root")
                continue;

            for (auto& [k, member] : struc.m_members)
            {
                m_permission_scope = member.m_permission.value();
                member.Resolve();
            }
            return true;
        }
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}