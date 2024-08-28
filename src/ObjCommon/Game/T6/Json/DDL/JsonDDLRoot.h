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

    //inline const std::unordered_map<std::string, int> DDL_TYPE_NAMES;

    //inline const std::unordered_map<std::string, int> DDL_PERM_NAMES;

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

    class JsonDDLMemberDef
    {
    public:
        std::string name;
        std::optional<size_t> size;
        std::optional<size_t> offset;
        std::optional<ddlPrimitiveTypes_e> type;
        std::optional<size_t> externalIndex;
        std::optional<JsonDDLMemberLimits> limits;
        std::optional<size_t> arraySize;
        std::optional<size_t> enumIndex;
        ddlPermissionTypes_e permission;
        std::optional<std::string> enum_;
        std::optional<std::string> struct_;
        std::optional<size_t> maxCharacters;
        
        mutable std::string parentStruct;
        mutable bool calculated;
        mutable size_t referenceCount;

        class Exception : public JsonDDLParseException
        {
        public:
            Exception(std::string& message);
        };

        void LogicError(const std::string& message) const;
        const bool IsStandardSize() const;
        const std::string& PermissionTypeToName() const noexcept;
        const std::string& TypeToName() const noexcept;
        ddlPrimitiveTypes_e NameToType(const std::string& typeName) const noexcept;
        ddlPermissionTypes_e NameToPermissionType(const std::string& typeName) const noexcept;
        void ReportCircularDependency(const JsonDDLDef& jDDLDef, const std::string message) const;
        void Validate(const JsonDDLDef& jDDLDef) const;
        void Calculate(JsonDDLDef& jDDLDef);

    private:
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
        JsonDDLMemberDef, name, size, offset, type, externalIndex, limits, arraySize, enumIndex, permission, enum_, struct_, maxCharacters);

    class JsonDDLStructDef
    {
    public:
        std::string name;
        std::optional<size_t> size;
        std::vector<JsonDDLMemberDef> members;
        std::map<int, int> hashTable;

        mutable std::string parentDef;
        mutable size_t referenceCount;
        mutable bool calculated;

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

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLStructDef, name, size, members, hashTable);

    class JsonDDLEnumDef
    {
    public:
        std::string name;
        std::vector<std::string> members;
        std::map<int, int> hashTable;

        mutable std::string parentDef;
        mutable size_t referenceCount;
        mutable bool calculated;

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

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLEnumDef, name, members, hashTable);

    class JsonDDLDef
    {
    public:
        int version;
        std::optional<size_t> size;
        std::vector<JsonDDLStructDef> structs;
        std::vector<JsonDDLEnumDef> enums;
        
        std::string filename;
        mutable std::vector<JsonDDLMemberDef> memberStack;
        mutable std::vector<bool> inCalculation;

        void LogicError(const std::string& message) const;
        std::optional<size_t> TypeToStructIndex(const std::string& typeName) const;
        std::optional<size_t> TypeToEnumIndex(const std::string& typeName) const;
        std::optional<std::string> StructIndexToType(const size_t index) const;
        std::optional<std::string> EnumIndexToType(const size_t index) const;
        void Validate() const;
        void ValidateRoot() const;
        void Calculate();
        
        class Exception : public JsonDDLParseException
        {
        public:
            Exception(std::string& message);
        };

    private:
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLDef, version, size, structs, enums);

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
        std::optional<std::vector<JsonDDLDef>> defs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLRoot, defFiles, defs);
} // namespace T6
