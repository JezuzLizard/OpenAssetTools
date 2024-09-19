#pragma once

#include "CommonDDL.h"

class CommonDDLDef;

class CommonDDLEnumDef
{
public:
    friend class CommonDDLDef;
    const std::string m_name;
    const bool m_from_include;
    const size_t m_index;
    std::vector<std::string> m_members;
    std::optional<std::string> m_include_file;

    [[noreturn]] void LogicError(const std::string& message) const;
    void Validate() const;
    void ReferenceCount() const;
    virtual void CalculateHashes(){};

    void SetCalculated();
    const bool IsCalculated() const;
    void ResetCalculated();
    const size_t GetRefCount() const;
    CommonDDLDef& GetParentDef();
    const CommonDDLDef& GetParentDef() const;

    std::vector<DDLHashEntry>& GetHashTable();
    const std::vector<DDLHashEntry>& GetHashTable() const;

    CommonDDLEnumDef();
    CommonDDLEnumDef(const std::string& name, CommonDDLDef* parent, const size_t index);
    CommonDDLEnumDef(const std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile);

private:
    CommonDDLDef* m_parent;
    size_t m_reference_count = 0;
    bool m_resolved = false;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    void ValidateName() const;
    void ValidateMembers() const;
};