
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

CommonDDLDef& CommonDDLEnumDef::GetParentDef()
{
    return *m_parent;
}

const CommonDDLDef& CommonDDLEnumDef::GetParentDef() const
{
    return *m_parent;
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

std::vector<DDLHashEntry>& CommonDDLEnumDef::GetHashTable()
{
    const auto& featureLevel = GetParentDef().GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

const std::vector<DDLHashEntry>& CommonDDLEnumDef::GetHashTable() const
{
    const auto& featureLevel = GetParentDef().GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

[[noreturn]] void CommonDDLEnumDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL Enum Logic Error: [Enum: {}|File: {}]", m_name, GetParentDef().m_filename) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLEnumDef::Validate() const
{
    if (m_calculated)
        return;
    ValidateMembers();
    ValidateName();

    m_calculated = true;
}

void CommonDDLEnumDef::ValidateName() const
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

        // This only occurs in unreferenced enums
        // LogicError(std::format("multiple occurrences of enum member \"{}\"", members[i]));
        std::cerr << std::format("multiple occurrences of enum member \"{}\"", m_members[i]) << "\n";
    }
}

void CommonDDLEnumDef::ValidateMembers() const
{
    if (m_members.empty())
        LogicError("enum must have at least one member");
}

void CommonDDLEnumDef::ReferenceCount() const
{
    if (!m_reference_count)
        std::cerr << "an unreferenced enum cannot be linked"
                  << "\n";
    // LogicError("an unreferenced enum cannot be linked");
}