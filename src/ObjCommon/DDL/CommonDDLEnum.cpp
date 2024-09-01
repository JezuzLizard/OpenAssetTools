#include "CommonDDL.h"
#include "CommonDDLDef.h"
#include "Utils/Alignment.h"

CommonDDLEnumDef::CommonDDLEnumDef(CommonDDLDef& parent)
    : m_parent(parent)
{
}

CommonDDLDef& CommonDDLEnumDef::GetParent()
{
    return m_parent;
}

const CommonDDLDef& CommonDDLEnumDef::GetParentConst() const
{
    return m_parent;
}

const size_t CommonDDLEnumDef::GetRefCount() const
{
    return m_reference_count;
}

void CommonDDLEnumDef::SetCalculated()
{
    m_calculated = true;
}

const bool CommonDDLEnumDef::IsCalculated()
{
    return m_calculated;
}

void CommonDDLEnumDef::ResetCalculated()
{
    m_calculated = false;
}

std::vector<DDLHash>& CommonDDLEnumDef::GetHashTable()
{
    return m_hash_table;
}

void CommonDDLEnumDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", m_name, GetParentConst()) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw CommonDDLEnumDef::Exception(prefaceAndMessage);
}

void CommonDDLEnumDef::Validate() const
{
    m_reference_count++;
    if (m_calculated)
        return;
    ValidateMembers();
    ValidateName();

    m_calculated = true;
}

void CommonDDLEnumDef::CalculateHashes()
{
}

CommonDDLEnumDef::Exception::Exception(std::string& message)
    : JsonDDLParseException::JsonDDLParseException(message)
{
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
