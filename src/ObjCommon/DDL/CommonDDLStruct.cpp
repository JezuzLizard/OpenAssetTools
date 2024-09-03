#include "CommonDDL.h"

CommonDDLStructDef::CommonDDLStructDef(CommonDDLDef& parent)
    : m_parent(parent)
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

const CommonDDLDef& CommonDDLStructDef::GetParentConst() const
{
    return m_parent;
}

CommonDDLDef& CommonDDLStructDef::GetParent()
{
    return m_parent;
}

const size_t CommonDDLStructDef::GetRefCount() const
{
    return m_reference_count;
}

std::vector<DDLHashEntry>& CommonDDLStructDef::GetHashTable()
{
    return m_hash_table;
}

void CommonDDLStructDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL Struct Logic Error: [Struct: {} | File: {}]", m_name, GetParentConst().m_filename) + message;
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

    CalculateHashes();

    auto size = 0u;
    for (auto i = 0u; i < m_members.size(); i++)
    {
        if (m_name == "root")
            m_permission_scope = m_members[i].m_permission.value_or(0);
        m_members[i].Calculate();
        m_members[i].m_link_data.m_offset = size;
        size += m_members[i].m_link_data.m_size;
    }

    m_size = size;

    m_calculated = true;
}

void CommonDDLStructDef::ValidateName() const
{
    std::string primary = m_name;
    utils::MakeStringLowerCase(primary);
    std::string other;

    GetParentConst().ValidateName(m_name.GetLowerConst());

    size_t nameRedefinitions = 0;
    for (const auto& struc : GetParentConst().m_structs)
    {
        if (struc.m_name.GetLowerConst() == m_name.GetLowerConst())
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("multiple redefinitions of struct");
    }

    for (const auto& enum_ : GetParentConst().m_structs)
    {
        if (enum_.m_name.GetLowerConst() == m_name.GetLowerConst())
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("struct cannot have same name as enum");
    }
}

void CommonDDLStructDef::ValidateMembers() const
{
    if (m_members.empty())
        LogicError("struct must have at least one member");

    for (const auto& member : m_members)
    {
        member.Validate();
    }
}

void CommonDDLStructDef::ReferenceCount() const
{
    if (!m_reference_count)
        std::cerr << std::format("[Def: {}|Name: {}] an unreferenced struct will not be used by the engine!", GetParentConst().m_filename, m_name)
                  << "\n";
    // LogicError("an unreferenced struct cannot be linked");
}
