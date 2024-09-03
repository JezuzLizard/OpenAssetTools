#pragma once
#include "Utils/StringUtils.h"

#include <unordered_set>
#include <set>
#include <format>
#include <iostream>
#include <cassert>
#include <algorithm>

constexpr auto OAT_DDL_FLOAT_BITS = (sizeof(float) * CHAR_BIT);

enum ddlTypeFlags_e : size_t
{
    DDL_FLAG_SIGNED = 0x1,
    DDL_FLAG_LIMITS = 0x2,
    DDL_FLAG_BITFIELDS = 0x4,
};

enum ddlUserDefinedTypeFlags_e : size_t
{
    DDL_USER_TYPE_NONE = 0x0,
    DDL_USER_TYPE_ENUM = 0x1,
    DDL_USER_TYPE_STRUCT = 0x2,
    DDL_USER_TYPE_BOTH = 0x3,
};

typedef int ddlHash;
typedef int ddlHashIndex;

class DDLHashEntry
{
public:
    ddlHash hash;
    ddlHashIndex index;
};

class DDLTypeFeature
{
public:
    size_t size;
    size_t flags;
    uint64_t max;
    int64_t min;
};

class DDLGameFeatures
{
public:
    const bool m_simple_hash_table;
    const bool m_split_hash_table;
    const bool m_header;
    const bool m_padding;
    const bool m_uint_type;
    const bool m_fixed_point_type;
    const bool m_bool_type;
};

class DDLString : public std::string
{
public:
    DDLString() : std::string()
    {
    }

    DDLString GetLower()
    {
        DDLString copy = *this;
        utils::MakeStringLowerCase(copy);
        return copy;
    }

    DDLString GetUpper()
    {
        DDLString copy = *this;
        utils::MakeStringUpperCase(copy);
        return copy;
    }

    const DDLString GetLowerConst() const
    {
        DDLString copy = *this;
        utils::MakeStringLowerCase(copy);
        return copy;
    }

    const DDLString GetUpperConst() const
    {
        DDLString copy = *this;
        utils::MakeStringUpperCase(copy);
        return copy;
    }

    std::string stdString;

private:
};

// This isn't strictly necessary, but I'm doing this anyway just in case someone wanted to transpile to the official formats.
// By maintaining the same standards the canonical DDL linking that the T7 mod tools linker has, avoids potential future issues if someone wanted to link a
// dumped DDL dumped using the json code here.
inline const std::unordered_set<std::string> DDL_KEYWORDS = {
    "struct",
    /*"root",*/
    "enum",
    /*"version",*/ // Canonically used in a member name.
    "bool",
    "byte",
    "short",
    "uint",
    "int",
    "uint64",
    "float",
    "fixed",
    "string",
    "codeversion"
    "checksum",
    "nopadding",
    "userflags",
    "ddlchecksum",
    "reserve",
    "server",
    "client",
    "both",
    "",
};

namespace DDL
{
    class Exception : public std::runtime_error
    {
    public:
        Exception(std::string& message) 
            : std::runtime_error(message)
        {

        }
    };
} // namespace DDL

#include "CommonDDLDef.h"
#include "CommonDDLEnum.h"
#include "CommonDDLStruct.h"
#include "CommonDDLMember.h"
#include "CommonDDLRoot.h"