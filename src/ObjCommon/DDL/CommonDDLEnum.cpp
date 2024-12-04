
#include "CommonDDL.h"
#include "CommonDDLDef.h"
#include "CommonDDLEnum.h"
#include <set>
#include <format>
#include <iostream>
#include <cassert>

CommonDDLEnumDef::CommonDDLEnumDef()
    : m_parent(0),
      m_name({0}),
      m_from_include(false),
      m_include_file(std::nullopt),
      m_index(0)
{
}

CommonDDLEnumDef::CommonDDLEnumDef(const std::string& name, CommonDDLDef* parent, const size_t index)
    : m_parent(parent),
      m_name(name),
      m_from_include(false),
      m_include_file(std::nullopt),
      m_index(index)
{
}

CommonDDLEnumDef::CommonDDLEnumDef(const std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile)
    : m_parent(parent),
      m_name(name),
      m_from_include(true),
      m_include_file(includeFile),
      m_index(index)
{
}

CommonDDLDef* CommonDDLEnumDef::GetParentDef()
{
    return m_parent;
}

const CommonDDLDef* CommonDDLEnumDef::GetParentDef() const
{
    return m_parent;
}

void CommonDDLEnumDef::SetParentDef(CommonDDLDef* parent)
{
    m_parent = parent;
}

const size_t CommonDDLEnumDef::GetRefCount() const
{
    return m_reference_count;
}

void CommonDDLEnumDef::SetCalculated()
{
    m_calculated = true;
}

const bool CommonDDLEnumDef::IsCalculated() const
{
    return m_calculated;
}

void CommonDDLEnumDef::ResetCalculated()
{
    m_calculated = false;
}

void CommonDDLEnumDef::CalculateHashes()
{
    if (IsCalculated())
        return;

    for (auto i = 0u; i < m_members.size(); i++)
    {
        DDLHashEntry ddlHash = {};
        auto func = [](const char* str)
        {
            if (!str)
                return 0;

            auto result = 0x1505;
            auto offset = 0;
            while (str[offset])
            {
                const auto c = tolower(str[offset++]);
                result = c + 33 * result;
            }

            return result;
        };
        ddlHash.hash = func(m_members[i].c_str());
        ddlHash.index = i;
        GetHashTable().emplace_back(ddlHash);
    }

    std::sort(GetHashTable().begin(),
              GetHashTable().end(),
              [](const DDLHashEntry& a, const DDLHashEntry& b)
              {
                  return a.hash < b.hash;
              });

#ifdef DDL_FATAL_ERROR_ON_CANON_UNDEFINED_BEHAVIOR
    CheckHashCollisions();
#endif

    SetCalculated();
}

void CommonDDLEnumDef::CheckHashCollisions()
{
    auto prevHash = 0;
    auto prevIndex = 0;
    for (auto i = 0; i < GetHashTable().size(); i++)
    {
        if (prevHash != 0 && GetHashTable()[i].hash == prevHash)
        {
            LogicError(std::format("hash collisions are not allowed within the same struct, detected between members: {} {}; please change one of the names",
                                   m_members[prevIndex],
                                   m_members[GetHashTable()[i].index]));
        }
        prevHash = GetHashTable()[i].hash;
        prevIndex = GetHashTable()[i].index;
    }
}

std::vector<DDLHashEntry>& CommonDDLEnumDef::GetHashTable()
{
    const auto& featureLevel = GetParentDef()->GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

const std::vector<DDLHashEntry>& CommonDDLEnumDef::GetHashTable() const
{
    const auto& featureLevel = GetParentDef()->GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

[[noreturn]] void CommonDDLEnumDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL ENUM ERROR: [Enum: {}|File: {}]", m_name, GetParentDef()->m_filename) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

const void CommonDDLEnumDef::Validate() const
{
    if (m_calculated)
        return;
    ValidateMembers();
    ValidateName();

    m_calculated = true;
}

const void CommonDDLEnumDef::ValidateName() const
{
    std::set<std::string> occurrences;

    auto i = 0u;
    for (; i < m_members.size(); i++)
    {
        if (!occurrences.contains(m_members[i]))
        {
            occurrences.insert(m_members[i]);
            continue;
        }

        //the game may depend on this undefined behavior
        std::cerr << std::format("DDL ENUM WARNING: [Def: {}|Enum: {}] multiple occurrences of enum member \"{}\"", GetParentDef()->m_filename, m_name, m_members[i]) << "\n";
    }
}

const void CommonDDLEnumDef::ValidateMembers() const
{
    if (m_members.empty())
        LogicError("enum must have at least one member");
}

const void CommonDDLEnumDef::ReferenceCount() const
{
    if (!m_reference_count)
        std::cerr << std::format("DDL ENUM WARNING: [Def: {}|Name: {}] an unreferenced enum will not be used by the engine!", GetParentDef()->m_filename, m_name)
                  << "\n";
    // LogicError("an unreferenced enum cannot be linked");
}