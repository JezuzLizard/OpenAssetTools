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
      m_parent_def(0),
      m_from_include(false),
      m_include_file(std::nullopt),
      m_index(0)
{
}

CommonDDLStructDef::CommonDDLStructDef(std::string& name, CommonDDLDef* parent, const size_t index)
    : m_name(std::move(name)),
      m_parent_def(parent),
      m_from_include(false),
      m_include_file(std::nullopt),
      m_index(index)
{
    assert(m_index == 0 || m_name != "root");
}

CommonDDLStructDef::CommonDDLStructDef(std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile)
    : m_name(std::move(name)),
      m_parent_def(parent),
      m_from_include(true),
      m_include_file(includeFile),
      m_index(index)
{
    assert(m_index == 0 || m_name != "root");
}

CommonDDLStructDef::~CommonDDLStructDef()
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

const CommonDDLDef* CommonDDLStructDef::GetParentDef() const
{
    return m_parent_def;
}

CommonDDLDef* CommonDDLStructDef::GetParentDef()
{
    return m_parent_def;
}

void CommonDDLStructDef::SetParentDef(CommonDDLDef* parent)
{
    m_parent_def = parent;
}

const size_t CommonDDLStructDef::GetRefCount() const
{
    return m_reference_count;
}

void CommonDDLStructDef::CalculateHashes()
{
    auto i = 0;
    for (const auto& member : m_members)
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
        ddlHash.hash = func(member.m_name.c_str());
        ddlHash.index = i;
        GetHashTable().push_back(ddlHash);
        i++;
    }

    std::sort(GetHashTable().begin(),
              GetHashTable().end(),
              [](const DDLHashEntry& a, const DDLHashEntry& b)
              {
                  return a.hash < b.hash;
              });
}

std::vector<DDLHashEntry>& CommonDDLStructDef::GetHashTable()
{
    const auto& featureLevel = GetParentDef()->GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

const std::vector<DDLHashEntry>& CommonDDLStructDef::GetHashTable() const
{
    const auto& featureLevel = GetParentDef()->GetFeatures();
    assert(featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table);

    return m_hash_table;
}

[[noreturn]] void CommonDDLStructDef::LogicError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("DDL Struct Logic Error: [Struct: {} | File: {}]", m_name, GetParentDef()->m_filename) + message;
#ifdef DDL_DEBUG
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

const void CommonDDLStructDef::Validate() const
{
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

    const auto& featureLevel = GetParentDef()->GetFeatures();
    if (featureLevel.m_simple_hash_table || featureLevel.m_split_hash_table)
    {
        CalculateHashes();
    }
    
    auto size = 0u;
    for (auto& member : m_members)
    {
        member.Calculate();
        member.m_offset = size;
        size += member.m_size;
    }

    m_size = size;

    m_calculated = true;
}

const void CommonDDLStructDef::ValidateName() const
{
    GetParentDef()->ValidateName(false, m_name);

    size_t nameRedefinitions = 0;
    for (const auto& struc : GetParentDef()->m_structs)
    {
        if (struc.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("multiple redefinitions of struct");
    }

    for (const auto& enum_ : GetParentDef()->m_enums)
    {
        if (enum_.m_name == m_name)
            nameRedefinitions++;

        if (nameRedefinitions > 1)
            LogicError("struct cannot have same name as enum");
    }
}

const void CommonDDLStructDef::ValidateMembers() const
{
    if (m_members.empty())
        LogicError("struct must have at least one member");

    for (const auto& member : m_members)
    {
        assert(member.m_type_category != (size_t)-1);
        member.Validate();
    }
}

const void CommonDDLStructDef::ReferenceCount() const
{
    if (!m_reference_count)
        std::cerr << std::format("DDL STRUCT WARNING: [Def: {}|Name: {}] an unreferenced struct will not be used by the engine!", GetParentDef()->m_filename, m_name)
                  << "\n";
    // LogicError("an unreferenced struct cannot be linked");
}

void CommonDDLStructDef::Resolve(bool referencedByRoot)
{
    for (auto& member : m_members)
    {
        member.Resolve(referencedByRoot);
    }
}