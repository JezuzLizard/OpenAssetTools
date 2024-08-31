#pragma once

#include "Game/T6/T6.h"

#include "Json/JsonCommon.h"
#include "Json/JsonExtension.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#include <unordered_set>
#include <variant>

namespace T6
{
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
        DDL_PERM_UNSPECIFIED,
        DDL_PERM_CLIENT,
        DDL_PERM_SERVER,
        DDL_PERM_BOTH,
        DDL_PERM_COUNT,
    };

    enum ddlTypeFlags_e : size_t
    {
        DDL_FLAG_SIGNED = 0x1,
        DDL_FLAG_LIMITS = 0x2,
        DDL_FLAG_BITFIELDS = 0x4,
    };

    /*
    NLOHMANN_JSON_SERIALIZE_ENUM(ddlPrimitiveTypes_e,
                                 {
                                     {DDL_BYTE_TYPE,       "byte"       },
                                     {DDL_SHORT_TYPE,      "short"      },
                                     {DDL_UINT_TYPE,       "uint"       },
                                     {DDL_INT_TYPE,        "int"        },
                                     {DDL_UINT64_TYPE,     "uint64"     },
                                     {DDL_FLOAT_TYPE,      "float"      },
                                     {DDL_FIXEDPOINT_TYPE, "fixed_float"},
                                     {DDL_STRING_TYPE,     "string"     },
                                     {DDL_STRUCT_TYPE,     "struct"     },
                                     {DDL_ENUM_TYPE,       "enum"       },
    });
    */
    NLOHMANN_JSON_SERIALIZE_ENUM(ddlPermissionTypes_e,
                                 {
                                     {DDL_PERM_UNSPECIFIED, "unspecified"},
                                     {DDL_PERM_CLIENT,      "client"     },
                                     {DDL_PERM_SERVER,      "server"     },
                                     {DDL_PERM_BOTH,        "both"       },
    });

    class JsonDDLDef;

    class DDLFeatureSupport
    {
    public:
        size_t size;
        size_t flags;
        uint64_t max;
        int64_t min;
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
        {"unspecified", DDL_PERM_UNSPECIFIED},
        {"client",      DDL_PERM_CLIENT     },
        {"server",      DDL_PERM_SERVER     },
        {"both",        DDL_PERM_BOTH       },
        {"",            DDL_PERM_COUNT      }
    };

    inline const DDLFeatureSupport DDL_TYPE_FEATURES[] = {
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
         .size = sizeof(int) * CHAR_BIT,
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

    // This isn't strictly necessary, but I'm doing this anyway just in case someone wanted to transpile to the official formats.
    // By maintaining the same standards the canonical DDL linking that the T7 mod tools linker has, avoids potential future issues if someone wanted to link a
    // dumped DDL dumped using the json code here.
    inline const std::unordered_set<std::string> DDL_KEYWORDS = {
        "struct",
        /*"root",*/
        "enum",
        /*"version",*/ //Canonically used in a member name.
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

    class JsonDDLParseException : public std::runtime_error
    {
    public:
        JsonDDLParseException(std::string& message);


    };

    class NameException : public JsonDDLParseException
    {
    public:
        NameException(std::string& message);
    };

    class JsonDDLMemberLimits
    {
    public:
        // Represents the size of the member in bits.
        // Used by the bitfield syntax to explicitly define the size.
        std::optional<size_t> bits;
        // Represents the size of the member based on the magnitude of the value of .range.
        std::optional<size_t> range;
        // Represents the size of the member split between precision bits, and magnitude bits.
        std::optional<size_t> fixedPrecisionBits;
        // Represents the size of the member split between precision bits, and magnitude bits.
        std::optional<size_t> fixedMagnitudeBits;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLMemberLimits, bits, range, fixedPrecisionBits, fixedMagnitudeBits);

    class JsonDDLMemberDefLinkData
    {
    public:
        int size = 0;
        int offset = 0;
        ddlPrimitiveTypes_e typeEnum = DDL_TYPE_COUNT;
        int externalIndex = 0;
        int enumIndex = -1;
        std::optional<std::string> struct_;
        std::optional<ddlPermissionTypes_e> permissionScope;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLMemberDefLinkData, size, offset, typeEnum, externalIndex, enumIndex, struct_, permissionScope);

    class JsonDDLMemberDef
    {
    public:
        std::string name;
        std::string type;
        std::optional<JsonDDLMemberLimits> limits;
        std::optional<int> arraySize;
        std::optional<ddlPermissionTypes_e> permission;
        std::optional<std::string> enum_;
        std::optional<int> maxCharacters;
        std::optional<JsonDDLMemberDefLinkData> linkData;
        
        mutable std::string parentStruct;
        mutable std::optional<size_t> referenceCount;
        mutable bool calculated = false;

        class Exception : public JsonDDLParseException
        {
        public:
            Exception(std::string& message);
        };

        void LogicError(const std::string& message) const;
        const bool IsStandardSize() const;
        const std::string& PermissionTypeToName() const noexcept;
        const std::string& TypeToName() const noexcept;
        ddlPrimitiveTypes_e NameToType() const noexcept;
        ddlPermissionTypes_e NameToPermissionType(const std::string& typeName) const noexcept;
        void ReportCircularDependency(const JsonDDLDef& jDDLDef, const std::string message) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void Calculate(JsonDDLDef& jDDLDef);

    private:
        void InheritPermission(JsonDDLDef& jDDLDef);
        void ValidateName(const JsonDDLDef& jDDLDef) const;
        void ValidateType(const JsonDDLDef& jDDLDef) const;
        void ValidatePermission(const JsonDDLDef& jDDLDef) const;
        void ValidateArray(const JsonDDLDef& jDDLDef) const;
        void ValidateMaxCharacters(const JsonDDLDef& jDDLDef) const;
        void ValidateEnum_(const JsonDDLDef& jDDLDef) const;
        void ValidateBitFields() const;
        void ValidateRange() const;
        void ValidateFixedPoint() const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(
        JsonDDLMemberDef, name, type, limits, arraySize, permission, enum_, maxCharacters);
    //NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(
        //JsonDDLMemberDef, name, size, offset, type, externalIndex, limits, arraySize, enumIndex, permission, enum_, struct_, maxCharacters);

    class JsonDDLStructDef
    {
    public:
        std::string name;
        std::optional<int> size;
        std::vector<JsonDDLMemberDef> members;
        std::vector<ddlHash_t> sortedHashTable;

        mutable std::string parentDef;
        mutable std::optional<size_t> referenceCount = 0;
        mutable bool calculated = false;

        void LogicError(const std::string& message) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void ReferenceCount(const JsonDDLDef& jDDLDef) const;
        void CalculateHashes();
        void Calculate(JsonDDLDef& jDDLDef);

        class Exception : public JsonDDLParseException
        {
        public:
            Exception(std::string& message);
        };

    private:

        void ValidateName(const JsonDDLDef& jDDLDef) const;
        void ValidateMembers(const JsonDDLDef& jDDLDef) const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLStructDef, name, size, members);

    class JsonDDLEnumDef
    {
    public:
        std::string name;
        std::vector<std::string> members;
        std::vector<ddlHash_t> sortedHashTable;

        mutable std::string parentDef;
        mutable std::optional<size_t> referenceCount = 0;
        mutable bool calculated = false;

        void LogicError(const std::string& message) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void ReferenceCount(const JsonDDLDef& jDDLDef) const;
        void CalculateHashes();

        class Exception : public JsonDDLParseException
        {
        public:
            Exception(std::string& message);
        };

    private:
        void ValidateName(const JsonDDLDef& jDDLDef) const;
        void ValidateMembers(const JsonDDLDef& jDDLDef) const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLEnumDef, name, members);

    class JsonDDLDef
    {
    public:
        int version;
        std::optional<int> size;
        std::vector<JsonDDLEnumDef> enums;
        std::vector<JsonDDLStructDef> structs;
        
        std::string filename;
        mutable std::vector<JsonDDLMemberDef> memberStack;
        mutable std::vector<bool> inCalculation;

        void LogicError(const std::string& message) const;
        size_t TypeToStructIndex(const std::string& typeName) const noexcept;
        int TypeToEnumIndex(const std::string& typeName) const noexcept;
        std::optional<std::string> StructIndexToType(const size_t index) const noexcept;
        std::optional<std::string> EnumIndexToType(const int index) const noexcept;
        bool Validate() const;
        void ValidateRoot() const;
        void PreCalculate() const;
        bool Calculate();
        
        class Exception : public JsonDDLParseException
        {
        public:
            Exception(std::string& message);
        };

    private:
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLDef, version, size, enums, structs);

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
        std::vector<JsonDDLDef> defs;
        std::vector<std::string> includeFiles;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLRoot, defFiles);
} // namespace T6
