#include "CommonDDL.h"
#include "CommonDDLDef.h"
#include "CommonDDLStruct.h"
#include "CommonDDLEnum.h"
#include "CommonDDLMember.h"

#include <cassert>
#include <format>
#include <iostream>

CommonDDLStructDef::CommonDDLStructDef()
    : m_name({0}),
      m_parent(0)
{
}

CommonDDLStructDef::CommonDDLStructDef(std::string& name, CommonDDLDef* parent)
    : m_name(std::move(name)),
    m_parent(parent)
{
}

void CommonDDLStructDef::SetCalculated()
{
    m_calculated = true;
}

void CommonDDLStructDef::ResetCalculated()
{
    m_calculated = false;
}

const CommonDDLDef& CommonDDLStructDef::GetParent() const
{
    return *m_parent;
}

CommonDDLDef& CommonDDLStructDef::GetParent()
{
    return *m_parent;
}

const size_t CommonDDLStructDef::GetRefCount() const
{
    return m_reference_count;
}

std::vector<DDLHashEntry>& CommonDDLStructDef::GetHashTable()
{
    const auto& featureLevel = GetParent().GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

const std::vector<DDLHashEntry>& CommonDDLStructDef::GetHashTable() const
{
    const auto& featureLevel = GetParent().GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

void CommonDDLStructDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL Struct Logic Error: [Struct: {} | File: {}]", m_name, GetParent().m_filename) + message;
#ifdef DDL_DEBUG
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLStructDef::Validate() const
{
    m_reference_count++;
    if (m_calculated)
        return;

    ValidateName();
    ValidateMembers();

    m_calculated = true;
}

void CommonDDLStructDef::Calculate()
{
    if (m_calculated)
        return;

    const auto& featureLevel = GetParent().GetFeatures();
    if (featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table)
    {
        CalculateHashes();
    }
    
    auto size = 0u;
    for (auto& [k, member] : m_members)
    {
        if (m_name == "root")
            m_permission_scope = member.m_permission.value_or(0);
        member.Calculate();
        member.m_link_data.m_offset = size;
        size += member.m_link_data.m_size;
    }

    m_size = size;

    m_calculated = true;
}

void CommonDDLStructDef::ValidateName() const
{
    GetParent().ValidateName(m_name);

    size_t nameRedefinitions = 0;
    for (const auto& [k, struc] : GetParent().m_structs)
    {
        if (struc.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("multiple redefinitions of struct");
    }

    for (const auto& [k, enum_] : GetParent().m_structs)
    {
        if (enum_.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("struct cannot have same name as enum");
    }
}

void CommonDDLStructDef::ValidateMembers() const
{
    if (m_members.empty())
        LogicError("struct must have at least one member");

    for (const auto& [k, member] : m_members)
    {
        member.Validate();
    }
}

void CommonDDLStructDef::ReferenceCount() const
{
    if (!m_reference_count)
        std::cerr << std::format("[Def: {}|Name: {}] an unreferenced struct will not be used by the engine!", GetParent().m_filename, m_name)
                  << "\n";
    // LogicError("an unreferenced struct cannot be linked");
}

void CommonDDLStructDef::Resolve()
{
    for (auto& [k, member] : m_members)
    {
        member.Resolve();
    }
}