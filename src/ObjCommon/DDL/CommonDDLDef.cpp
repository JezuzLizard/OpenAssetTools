
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
      m_filename(filename),
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
    auto it = std::find_if(m_structs.begin(),
                           m_structs.end(),
                           [structName](const CommonDDLStructDef& struc)
                           {
                               return structName == struc.m_name;
                           });
    if (it != m_structs.end())
    {
        return &m_structs[it->m_index];
    }

    assert(!checkIncludes);

    if (checkIncludes)
    {
        assert(GetRoot().m_include_pool.contains(m_filename));

        for (auto& include : GetRoot().m_include_pool[m_filename])
        {
            assert(include.m_is_include);

            auto it = std::find_if(include.m_structs.begin(),
                                   include.m_structs.end(),
                                   [structName](const CommonDDLStructDef& struc)
                                   {
                                       return structName == struc.m_name;
                                   });
            if (it != include.m_structs.end())
            {
                return &include.m_structs[it->m_index];
            }
        }
    }
    return nullptr;
}

CommonDDLEnumDef* CommonDDLDef::GetEnumByName(const std::string& enumName, bool checkIncludes)
{
    auto it = std::find_if(m_enums.begin(),
                 m_enums.end(),
                 [enumName](const CommonDDLEnumDef& _enum)
                 {
                     return enumName == _enum.m_name;                    
                 });
    if (it != m_enums.end())
    {
        return &m_enums[it->m_index];
    }

    if (checkIncludes)
    {
        assert(GetRoot().m_include_pool.contains(m_filename));

        for (auto& include : GetRoot().m_include_pool[m_filename])
        {
            assert(include.m_is_include);

            auto it = std::find_if(include.m_enums.begin(),
                                   include.m_enums.end(),
                                   [enumName](const CommonDDLEnumDef& _enum)
                                   {
                                       return enumName == _enum.m_name;
                                   });
            if (it != include.m_enums.end())
            {
                return &include.m_enums[it->m_index];
            }
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

   auto it = std::find_if(m_structs.begin(),
                           m_structs.end(),
                           [typeName](const CommonDDLStructDef& struc)
                           {
                               return typeName == struc.m_name;
                           });
    if (it != m_structs.end())
    {
        return it->m_index;
    }

    return 0u;
}

const int CommonDDLDef::TypeToEnumIndex(const std::string& typeName) const noexcept
{
    // canonical T7 linker behavior
    if (m_enums.size() == 0)
        return -1;

    auto it = std::find_if(m_enums.begin(),
                           m_enums.end(),
                           [typeName](const CommonDDLEnumDef& _enum)
                           {
                               return typeName == _enum.m_name;
                           });
    if (it != m_enums.end())
    {
        return it->m_index;
    }

    return -1;
}

const std::optional<std::reference_wrapper<const std::string>> CommonDDLDef::StructIndexToType(const size_t index) const noexcept
{
    auto i = 0u;
    for (const auto& struc : m_structs)
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
    for (const auto& enum_ : this->m_enums)
    {
        if (i == index)
        {
            return enum_.m_name;
        }
        i++;
    }

    return std::nullopt;
}

const bool CommonDDLDef::Validate() const
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
    for (auto& struc : m_structs)
    {
        struc.ResetCalculated();
        for (auto& member : struc.m_members)
            member.ResetCalculated();
    }
    for (auto& enum_ : m_enums)
        enum_.ResetCalculated();

    m_in_calculation.clear();
    m_in_calculation.resize(m_structs.size());
}

bool CommonDDLDef::Calculate()
{
    PreCalculate();

    try
    {
        for (auto& struc : m_structs)
        {
            if (struc.m_name == "root")
            {
                struc.Calculate();
                break;
            }
        }

        // Unreferenced structs can canonically be linked into the asset, but do not change the size of the ddl buffer as they are not in the root struct.
        // Treyarch likely had an include system that would paste structs from an external file into the the defs that include.
        // Or they had an export/import system.
        for (auto& struc : m_structs)
        {
            if (struc.m_name == "root")
            {
                m_size = struc.m_size;
                break;
            }
        }

        // Link unreferenced structs anyway
        // TODO: check if necessary for unreferenced structs/enums to be linked anyway
        for (auto& struc : m_structs)
            if (!struc.GetRefCount())
                struc.Calculate();

        // Link unreferenced enums anyway
        for (auto& enum_ : m_enums)
            if (!enum_.GetRefCount())
                enum_.CalculateHashes();

        for (auto& enum_ : m_enums)
            enum_.CalculateHashes();

        return true;
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}

const void CommonDDLDef::ValidateRoot() const
{
    for (const auto& struc : m_structs)
    {
        if (struc.m_name == "root")
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

    for (const auto& enum_ : m_enums)
    {
        enum_.ReferenceCount();
        if (!enum_.GetRefCount())
            enum_.Validate();
    }

    for (const auto& struc : m_structs)
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

const void CommonDDLDef::ValidateName(bool isType, const std::string& name) const
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

    if (name == "root")
        return;

    if (GetFeatures().m_version_keyword_allowed_as_name && name == "version")
        return;

    if (isType)
    {
        const auto& it = DDL_TYPE_NAMES.find(name);
        if (it != DDL_TYPE_NAMES.end())
            return;
    }

    const auto& it = DDL_KEYWORDS.find(name);
    if (it != DDL_KEYWORDS.end())
        NameError(std::format("field cannot be reserved keyword \"{}\"", it->data()));
}

void CommonDDLDef::AddStructFromInclude(CommonDDLStructDef& includeStruct)
{
    auto it = std::find_if(m_structs.begin(),
                           m_structs.end(),
                           [includeStruct](const CommonDDLStructDef& struc)
                           {
                               return includeStruct.m_name == struc.m_name;
                           });
    if (it == m_structs.end())
    {
        includeStruct.m_reference_count++;
        if (includeStruct.m_resolved)
            return;

        includeStruct.m_resolved = true;
        m_structs.push_back(includeStruct);
        includeStruct.Resolve(true);
    }
    else
        assert(false);
}

void CommonDDLDef::AddEnumFromInclude(CommonDDLEnumDef& includeEnum)
{
    auto it = std::find_if(m_enums.begin(),
                           m_enums.end(),
                           [includeEnum](const CommonDDLEnumDef& _enum)
                           {
                               return includeEnum.m_name == _enum.m_name;
                           });
    if (it != m_enums.end())
    {
        includeEnum.m_reference_count++;
        if (includeEnum.m_resolved)
            return;

        includeEnum.m_resolved = true;
        m_enums.push_back(includeEnum);
    }
    else
        assert(false);
}

bool CommonDDLDef::Resolve()
{
    try
    {
        // Determine what structs from what includes are referenced
        for (auto& struc : m_structs)
        {
            if (struc.m_name != "root")
                continue;

            struc.Resolve(true);

            // root should always only have exactly 1 reference
            assert(struc.m_name != "root" || struc.m_reference_count == 1);
        }

        for (auto& struc : m_structs)
        {
            if (struc.m_resolved)
            {
                assert(struc.m_reference_count != 0);
                continue;
            }

#ifdef DDL_FATAL_ERROR_ON_CANON_UNDEFINED_BEHAVIOR
            if (struc.m_reference_count == 0)
                LogicError(std::format("unreferenced struct {} cannot be linked", struc.m_name));
#else
            if (struc.m_reference_count == 0)
            {
                std::cerr << std::format("DDL STRUCT WARNING: unreferenced struct {} cannot be linked", struc.m_name) << "\n";
            }
            assert(struc.m_reference_count == 0);

            for (auto& member : struc.m_members)
            {
                member.Resolve(false);
            }
#endif
        }
        for (auto& enum_ : m_enums)
        {
#ifdef DDL_FATAL_ERROR_ON_CANON_UNDEFINED_BEHAVIOR
            if (enum_.m_reference_count == 0)
                LogicError(std::format("unreferenced enum {} cannot be linked", enum_.m_name));
#else
            if (enum_.m_reference_count == 0)
            {
                std::cerr << std::format("DDL ENUM WARNING: unreferenced enum {} cannot be linked", enum_.m_name) << "\n";
            }
#endif
        }

        return true;
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}