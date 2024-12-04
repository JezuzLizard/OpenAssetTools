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
    size_t m_reference_count = 0;

    [[noreturn]] void LogicError(const std::string& message) const;
    const void Validate() const;
    const void ReferenceCount() const;
    void CalculateHashes();
    void CheckHashCollisions();

    void SetCalculated();
    const bool IsCalculated() const;
    void ResetCalculated();
    const size_t GetRefCount() const;
    CommonDDLDef* GetParentDef();
    const CommonDDLDef* GetParentDef() const;
    void SetParentDef(CommonDDLDef* parent);

    std::vector<DDLHashEntry>& GetHashTable();
    const std::vector<DDLHashEntry>& GetHashTable() const;

    CommonDDLEnumDef();
    CommonDDLEnumDef(const std::string& name, CommonDDLDef* parent, const size_t index);
    CommonDDLEnumDef(const std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile);

private:
    CommonDDLDef* m_parent;
    bool m_resolved = false;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    const void ValidateName() const;
    const void ValidateMembers() const;
};