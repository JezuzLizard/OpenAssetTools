#pragma once

#include <unordered_set>
#include <unordered_map>
#include <stdexcept>
#include <optional>
#include <algorithm>

constexpr auto OAT_DDL_FLOAT_BITS = (sizeof(float) * CHAR_BIT);

enum ddlTypeFlags_e : size_t
{
    DDL_FLAG_SIGNED = 0x1,
    DDL_FLAG_LIMITS = 0x2,
    DDL_FLAG_BITFIELDS = 0x4,
};

enum ddlCategoryFlags_e : size_t
{
    DDL_CATEGORY_FLAG_NONE = 0x0,
    DDL_CATEGORY_FLAG_ENUM = 0x1,
    DDL_CATEGORY_FLAG_STRUCT = 0x2,
    DDL_CATEGORY_FLAG_BOTH = 0x3,
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
    const bool m_version_keyword_allowed_as_name;
};

// This isn't strictly necessary, but I'm doing this anyway just in case someone wanted to transpile to the official formats.
// By maintaining the same standards the canonical DDL linking that the T7 mod tools linker has, avoids potential future issues if someone wanted to link a
// dumped DDL dumped using the json code here.
inline const std::unordered_set<std::string> DDL_KEYWORDS = {
    "struct",
    "root",
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
    "include",
    "",
};

constexpr auto OAT_DDL_VERSION = 1u;

enum ddlPrimitiveTypes_e : size_t
{
    DDL_BYTE_TYPE,
    DDL_SHORT_TYPE,
    DDL_UINT_TYPE,
    DDL_INT_TYPE,
    DDL_UINT64_TYPE,
    DDL_FLOAT_TYPE,
    DDL_FIXEDPOINT_TYPE,
    DDL_STRING_TYPE,
    DDL_STRUCT_TYPE,
    DDL_ENUM_TYPE,
    DDL_TYPE_COUNT,
};

enum ddlPermissionTypes_e : size_t
{
    DDL_PERMISSIONS_UNSPECIFIED,
    DDL_PERMISSIONS_CLIENTONLY,
    DDL_PERMISSIONS_SERVERONLY,
    DDL_PERMISSIONS_BOTH,
    DDL_PERMISSIONS_COUNT,
};

inline const std::unordered_map<std::string, int> DDL_TYPE_NAMES = {
    {"byte",        DDL_BYTE_TYPE      },
    {"short",       DDL_SHORT_TYPE     },
    {"uint",        DDL_UINT_TYPE      },
    {"int",         DDL_INT_TYPE       },
    {"uint64",      DDL_UINT64_TYPE    },
    {"float",       DDL_FLOAT_TYPE     },
    {"fixed_float", DDL_FIXEDPOINT_TYPE},
    {"string",      DDL_STRING_TYPE    },
    {"struct",      DDL_STRUCT_TYPE    },
    {"enum",        DDL_ENUM_TYPE      },
    {"",            DDL_TYPE_COUNT     }
};

inline const std::unordered_map<std::string, int> DDL_PERM_NAMES = {
    {"unspecified", DDL_PERMISSIONS_UNSPECIFIED},
    {"client",      DDL_PERMISSIONS_CLIENTONLY },
    {"server",      DDL_PERMISSIONS_SERVERONLY },
    {"both",        DDL_PERMISSIONS_BOTH       },
    {"",            DDL_PERMISSIONS_COUNT      }
};

inline const DDLTypeFeature DDL_TYPE_FEATURES[6] = {
    {
     .size = sizeof(char) * CHAR_BIT,
     .flags = 0,
     .max = UCHAR_MAX,
     .min = 0,
     },
    {
     .size = sizeof(short) * CHAR_BIT,
     .flags = 0,
     .max = USHRT_MAX,
     .min = 0,
     },
    {
     .size = sizeof(size_t) * CHAR_BIT,
     .flags = DDL_FLAG_LIMITS | DDL_FLAG_BITFIELDS,
     .max = UINT_MAX,
     .min = 0,
     },
    {
     .size = sizeof(int) * CHAR_BIT,
     .flags = DDL_FLAG_SIGNED | DDL_FLAG_LIMITS | DDL_FLAG_BITFIELDS,
     .max = INT_MAX,
     .min = INT_MIN,
     },
    {
     .size = sizeof(uint64_t) * CHAR_BIT,
     .flags = 0,
     .max = UINT64_MAX,
     .min = 0,
     },
    {
     .size = sizeof(float) * CHAR_BIT,
     .flags = 0,
     .max = 0,
     .min = 0,
     }
};

inline const DDLGameFeatures DDL_GAME_FEATURES = {.m_simple_hash_table = true,
                                                  .m_split_hash_table = false,
                                                  .m_header = false,
                                                  .m_padding = false,
                                                  .m_uint_type = true,
                                                  .m_fixed_point_type = true,
                                                  .m_bool_type = false,
                                                  .m_version_keyword_allowed_as_name = true};

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