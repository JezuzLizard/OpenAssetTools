#pragma once

class CommonDDLStructDef
{
public:
    DDLString m_name;
    int m_size = -1;
    std::unordered_map<DDLString, CommonDDLMemberDef> m_members;
    int m_permission_scope;
    std::optional<DDLString> m_include_file;

    void LogicError(const std::string& message) const;
    void Validate() const;
    void ReferenceCount() const;
    virtual void CalculateHashes() {};
    void Calculate();

    void SetCalculated();
    void ResetCalculated();
    void ResetRefCount();
    const size_t GetRefCount() const;
    CommonDDLDef& GetParent();
    const CommonDDLDef& GetParent() const;
    std::vector<DDLHashEntry>& GetHashTable();
    const std::vector<DDLHashEntry>& GetHashTable() const;
    void Resolve();

    CommonDDLStructDef(CommonDDLDef& parent);

private:
    CommonDDLDef& m_parent;
    mutable size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    void ValidateName() const;
    void ValidateMembers() const;
};