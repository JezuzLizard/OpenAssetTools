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
    std::vector<CommonDDLMemberDef> m_members;
    std::optional<std::string> m_include_file;
    bool m_from_include;
    const size_t m_index;
    size_t m_reference_count = 0;

    [[noreturn]] void LogicError(const std::string& message) const;
    const void Validate() const;
    const void ReferenceCount() const;
    void CalculateHashes();
    void CheckHashCollisions();
    void Calculate();

    void SetCalculated();
    void ResetCalculated();
    const size_t GetRefCount() const;
    CommonDDLDef* GetParentDef();
    const CommonDDLDef* GetParentDef() const;
    void SetParentDef(CommonDDLDef* parent);
    std::vector<DDLHashEntry>& GetHashTable();
    const std::vector<DDLHashEntry>& GetHashTable() const;
    void Resolve(bool referencedByRoot);

    CommonDDLStructDef();
    CommonDDLStructDef(std::string& name, CommonDDLDef* parent, const size_t index);
    CommonDDLStructDef(std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile);
    ~CommonDDLStructDef();

private:
    CommonDDLDef* m_parent_def;
    bool m_resolved = false;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    const void ValidateName() const;
    const void ValidateMembers() const;
};