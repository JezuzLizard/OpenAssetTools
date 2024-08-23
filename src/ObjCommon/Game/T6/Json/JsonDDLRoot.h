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
  enum ddlPrimitiveTypes_e
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
    DDL_INVALID_TYPE = 0xFFFFFFFF,
  };

	inline const std::string DDL_TYPE_NAMES[]{
		"byte",
		"short",
		"uint",
		"int",
		"uint64",
		"float",
		"fixed_float",
		"string",
		"struct",
		"enum",
		"",
	};

  class JsonDDLMemberLimits
  {
    public:
    //bool usesBitfield;
    //bool usesRangeLimits;
    std::optional<int> bitCount;
    std::optional<unsigned int> rangeLimit;
		std::optional<unsigned int> serverDelta;
		std::optional<unsigned int> clientDelta;
    std::optional<std::bitset<32>> fixedPrecisionBits;
    std::optional<std::bitset<32>> fixedMagnitudeBits;
  }

  NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLMemberSize);

    class JsonDDLMemberDef
    {
    public:
		std::string name;
    std::optional<JsonDDLMemberLimits> limits;
		int offset;
		std::string type;
		std::optional<int> externalIndex;
		std::optional<int> arraySize;
		std::optional<int> enumIndex;
		std::optional<int> permission;
    };

	NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLMemberDef, name, size, offset, type, externalIndex, rangeLimit, serverDelta, clientDelta, arraySize, enumIndex, permission);

    class JsonDDLStructDef
    {
    public:
		std::string name;
		std::vector<JsonDDLMemberDef> members;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLStructDef, name, members);

    class JsonDDLEnumDef
    {
    public:
		std::string name;
		std::vector<std::string> members;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLEnumDef, name, members);

    class JsonDDLDef
    {
    public:
		int version;
		std::optional<std::vector<JsonDDLStructDef>> structs;
    std::optional<std::vector<JsonDDLEnumDef>> enums;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLDef, version, structs, enums);

    class JsonDDLRoot
    {
    public:
    std::vector<std::string> defFiles;
    std::vector<JsonDDLDef> defs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLRoot, defFiles, defs);
} // namespace T6