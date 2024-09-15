#pragma once

#include "CommonDDL.h"

class CommonDDLDef;

class CommonDDLEnumDef
{
public:
    std::string m_name;
    std::vector<std::string> m_members;
    std::optional<std::string> m_include_file;

    void LogicError(const std::string& message) const;
    void Validate() const;
    void ReferenceCount() const;
    virtual void CalculateHashes(){};

    void SetCalculated();
    const bool IsCalculated() const;
    void ResetCalculated();
    void ResetRefCount();
    const size_t GetRefCount() const;
    CommonDDLDef& GetParent();
    const CommonDDLDef& GetParent() const;

    std::vector<DDLHashEntry>& GetHashTable();
    const std::vector<DDLHashEntry>& GetHashTable() const;

    CommonDDLEnumDef(CommonDDLDef& parent);

private:
    CommonDDLDef& m_parent;
    mutable size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    void ValidateName() const;
    void ValidateMembers() const;
};