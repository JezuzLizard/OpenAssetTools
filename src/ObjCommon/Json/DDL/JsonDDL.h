#pragma once

#include "DDL/CommonDDL.h"

#include "Json/JsonCommon.h"
#include <nlohmann/json.hpp>

class CommonDDLMemberDefLinkData;

class JsonDDLMemberLimits
{
public:
    // Represents the size of the member in bits.
    // Used by the bitfield syntax to explicitly define the size.
    std::optional<size_t> bits;
    // Represents the size of the member based on the magnitude of the value of .range.
    std::optional<size_t> range;
    // T5 specific implementation of rangelimit
    std::optional<int> min;
    std::optional<int> max;
    // Represents the size of the member split between precision bits, and magnitude bits.
    std::optional<size_t> fixedPrecisionBits;
    // Represents the size of the member split between precision bits, and magnitude bits.
    std::optional<size_t> fixedMagnitudeBits;
};

NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLMemberLimits, bits, range, min, max, fixedPrecisionBits, fixedMagnitudeBits);

class JsonDDLMemberDef
{
public:
    std::string name;
    std::string type;
    std::optional<JsonDDLMemberLimits> limits;
    std::optional<int> arraySize;
    std::optional<std::string> permission;
    std::optional<std::string> enum_;
    std::optional<int> maxCharacters;
    CommonDDLMemberDefLinkData link;
    mutable size_t refCount = 0;
};

NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLMemberDef, name, type, limits, arraySize, permission, enum_, maxCharacters);

class JsonDDLStructDef
{
public:
    std::string name;
    std::optional<int> size;
    std::vector<JsonDDLMemberDef> members;
    std::vector<DDLHashEntry> sortedHashTable;
    mutable size_t refCount = 0;
};

NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLStructDef, name, members);

class JsonDDLEnumDef
{
public:
    std::string name;
    std::vector<std::string> members;
    std::vector<DDLHashEntry> sortedHashTable;
    mutable size_t refCount = 0;
};

NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLEnumDef, name, members);

class JsonDDLDef
{
public:
    std::string filename;
    int version;
    std::optional<int> size;
    std::vector<JsonDDLEnumDef> enums;
    std::vector<JsonDDLStructDef> structs;
    std::vector<std::string> includeFiles;
    mutable std::vector<bool> inCalculation;
    mutable std::string permissionScope;
};

NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLDef, includeFiles, version, enums, structs);

class JsonDDLRoot
{
public:
    std::string filename;
    std::vector<std::string> defFiles;
    std::vector<JsonDDLDef> defs;
    std::unordered_map<std::string, std::vector<JsonDDLDef>> includeDefs;
};

NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLRoot, defFiles);

