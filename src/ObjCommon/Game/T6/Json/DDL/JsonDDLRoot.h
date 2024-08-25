#pragma once

#include "Game/T6/T6.h"

#include "Json/JsonCommon.h"
#include "Json/JsonExtension.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

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

    inline const DDLFeatureSupport DDL_TYPE_FEATURES[];

    inline const std::string DDL_TYPE_NAMES[];

    inline const std::string DDL_PERM_NAMES[];

    class JsonDDLMemberLimits
    {
    public:
        std::optional<size_t> bits;
        std::optional<size_t> range;
        std::optional<size_t> fixedPrecisionBits;
        std::optional<size_t> fixedMagnitudeBits;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLMemberLimits, bits, range, fixedPrecisionBits, fixedMagnitudeBits);

    class JsonDDLMemberDef
    {
    public:
        std::string name;
        std::string type;
        std::optional<std::string> enum_;
        std::optional<size_t> enumIndex;
        std::string permission;
        std::optional<size_t> maxCharacters;
        std::optional<size_t> structIndex;
        std::optional<size_t> arrayCount;
        std::optional<size_t> memberSize;
        std::string parentStruct;
        std::optional<size_t> offset;
        std::optional<JsonDDLMemberLimits> limits;
        bool calculated;
        size_t referenceCount;

        class JsonDDLMemberDefException : public std::runtime_error
        {
        public:
            JsonDDLMemberDefException(std::string& message);
        };

        void MemberLogicError(const std::string& message) const;
        bool IsStandardSize(const size_t size, const ddlPrimitiveTypes_e type, const size_t arraySize) const;
        std::string PermissionTypeToName(const ddlPermissionTypes_e type) const;
        std::string TypeToName(const ddlPrimitiveTypes_e type) const;
        ddlPrimitiveTypes_e NameToType(const std::string& typeName) const;
        ddlPermissionTypes_e NameToPermissionType(const std::string& typeName) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void Calculate(JsonDDLDef& jDDLDef);

    private:
        void StoreParent(JsonDDLDef& jDDLDef);
        void Name(const JsonDDLDef& jDDLDef) const;
        void Type(const JsonDDLDef& jDDLDef) const;
        void Permission(const JsonDDLDef& jDDLDef) const;
        void Array(const JsonDDLDef& jDDLDef) const;
        void MaxCharacters(const JsonDDLDef& jDDLDef) const;
        void Enum_(const JsonDDLDef& jDDLDef) const;
        void BitFields() const;
        void Range() const;
        void FixedPoint() const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(
        JsonDDLMemberDef, name, type, enum_, permission, maxCharacters, offset, structIndex, arrayCount, enumIndex, memberSize, limits);

    class JsonDDLStructDef
    {
    public:
        std::string name;
        std::optional<size_t> structSize;
        std::vector<JsonDDLMemberDef> members;
        std::string parentDef;
        size_t referenceCount;
        bool calculated;
        std::map<size_t, size_t> hashTable;

        void StructLogicError(const std::string& message) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void CalculateHashes();
        void Calculate(JsonDDLDef& jDDLDef);

    private:
        class JsonDDLStructDefException : public std::runtime_error
        {
        public:
            JsonDDLStructDefException(std::string& message);
        };

        void Name(const JsonDDLDef& jDDLDef) const;
        void Members(const JsonDDLDef& jDDLDef) const;
        void ReferenceCount(const JsonDDLDef& jDDLDef) const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLStructDef, name, structSize, members, hashTable);

    class JsonDDLEnumDef
    {
    public:
        std::string name;
        std::vector<std::string> members;
        std::optional<std::string> parentDef;
        size_t referenceCount;
        bool calculated;
        std::map<int, int> hashTable;

        void EnumLogicError(const std::string& message) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void CalculateHashes();

    private:
        class JsonDDLEnumDefException : public std::runtime_error
        {
        public:
            JsonDDLEnumDefException(std::string& message);
        };

        void Name(const JsonDDLDef& jDDLDef) const;
        void Members(const JsonDDLDef& jDDLDef) const;
        void ReferenceCount(const JsonDDLDef& jDDLDef) const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLEnumDef, name, members, hashTable);

    class JsonDDLDef
    {
    public:
        std::optional<std::string> filename;
        int version;
        std::optional<size_t> defSize;
        std::vector<JsonDDLEnumDef> enums;
        std::vector<JsonDDLStructDef> structs;
        std::set<bool> inCalculation;

        void DefLogicError(const std::string& message) const;
        std::optional<size_t> TypeToStructIndex(const JsonDDLDef& jDDLDef, const std::string& typeName) const;
        std::optional<size_t> TypeToEnumIndex(const JsonDDLDef& jDDLDef, const std::string& typeName) const;
        std::optional<std::string> StructIndexToType(const JsonDDLDef& jDDLDef, const size_t index) const;
        std::optional<std::string> EnumIndexToType(const JsonDDLDef& jDDLDef, const size_t index) const;
        void Validate() const;
        void Calculate();
        
    private:
        class JsonDDLDefException : public std::runtime_error
        {
        public:
            JsonDDLDefException(std::string& message);
        };

        void Root() const;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLDef, version, structs, enums, defSize);

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
        std::vector<JsonDDLDef> defs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLRoot, defFiles, defs);
} // namespace T6
