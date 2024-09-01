#pragma once

#include "CommonDDL.h"
#include "Utils/ClassUtils.h"
#include <functional>

class CommonDDLEnumDef
{
public:
    DDLString m_name;
    std::vector<DDLString> m_members;

    void LogicError(const std::string& message) const;
    void Validate() const;
    void ReferenceCount() const;
    virtual void CalculateHashes(){};

    class Exception : public JsonDDLParseException
    {
    public:
        Exception(std::string& message);
    };

    void SetCalculated();
    const bool IsCalculated();
    void ResetCalculated();
    void ResetRefCount();
    const size_t GetRefCount() const;
    CommonDDLDef& GetParent();
    const CommonDDLDef& GetParentConst() const;

    std::vector<DDLHash>& GetHashTable();

    CommonDDLEnumDef(CommonDDLDef& parent);

private:
    CommonDDLDef& m_parent;
    mutable size_t m_reference_count = 0;
    mutable bool m_calculated = false;
    std::vector<DDLHash> m_hash_table;

    void ValidateName() const;
    void ValidateMembers() const;
};