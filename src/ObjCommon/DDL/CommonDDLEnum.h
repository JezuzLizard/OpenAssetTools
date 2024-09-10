#pragma once

class CommonDDLEnumDef
{
public:
    DDLString m_name;
    std::vector<DDLString> m_members;
    std::optional<DDLString> m_include_file;

    void LogicError(const std::string& message) const;
    void Validate() const;
    void ReferenceCount() const;
    virtual void CalculateHashes(){};

    void SetCalculated();
    const bool IsCalculated();
    void ResetCalculated();
    void ResetRefCount();
    const size_t GetRefCount() const;
    CommonDDLDef& GetParent();
    const CommonDDLDef& GetParentConst() const;

    std::vector<DDLHashEntry>& GetHashTable();

    CommonDDLEnumDef(CommonDDLDef& parent);

private:
    CommonDDLDef& m_parent;
    mutable size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    std::vector<DDLHashEntry> m_hash_table;

    void ValidateName() const;
    void ValidateMembers() const;
};