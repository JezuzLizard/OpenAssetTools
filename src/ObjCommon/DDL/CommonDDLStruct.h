#pragma once

#include "CommonDDL.h"

class CommonDDLDef;
class CommonDDLMemberDef;

class CommonDDLStructDef
{
public:
    friend class CommonDDLDef;
    const std::string m_name;
    int m_size = -1;
    std::unordered_map<std::string, CommonDDLMemberDef> m_members;
    std::optional<std::string> m_include_file;
    bool m_from_include;
    const size_t m_index;

    [[noreturn]] void LogicError(const std::string& message) const;
    void Validate() const;
    void ReferenceCount() const;
    virtual void CalculateHashes() {};
    void Calculate();

    void SetCalculated();
    void ResetCalculated();
    const size_t GetRefCount() const;
    CommonDDLDef& GetParentDef();
    const CommonDDLDef& GetParentDef() const;
    std::vector<DDLHashEntry>& GetHashTable();
    const std::vector<DDLHashEntry>& GetHashTable() const;
    void Resolve();

    CommonDDLStructDef();
    CommonDDLStructDef(std::string& name, CommonDDLDef* parent, const size_t index);
    CommonDDLStructDef(std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile);

private:
    CommonDDLDef* m_parent_def;
    size_t m_reference_count = 0;
    bool m_resolved = false;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    void ValidateName() const;
    void ValidateMembers() const;
};