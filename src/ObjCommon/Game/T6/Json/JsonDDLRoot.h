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

  enum ddlPermissionTypes_e
  {
    DDL_PERM_UNSPECIFIED,
    DDL_PERM_CLIENT,
    DDL_PERM_SERVER,
    DDL_PERM_BOTH,
    DDL_PERM_COUNT,
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

  	inline const std::string DDL_PERM_NAMES[]{
    "unspecified",
		"client",
		"server",
		"both",
		"",
	};

    class JsonDDLMemberLimits
    {
    public:
        std::optional<int> bits;
        std::optional<unsigned int> range;
        std::optional<int> fixedPrecisionBits;
        std::optional<int> fixedMagnitudeBits;
    };

  NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLMemberLimits, bits, range, fixedPrecisionBits, fixedMagnitudeBits);

    class JsonDDLMemberDef
    {
    public:
		std::string name;
        std::string type;
        std::string permission;
        std::optional<int> maxCharacters;
		std::optional<int> offset;
        std::optional<int> structIndex;
		std::optional<int> arrayCount;
        std::optional<int> enumIndex;
        std::optional<int> permissionEnum;
        std::optional<int> totalSize;
        std::optional<std::string> enum_;
        std::optional<JsonDDLMemberLimits> limits;
    };

	NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLMemberDef, name, type, permission, maxCharacters, offset, structIndex, arrayCount, enumIndex, permissionEnum, totalSize, enum_, limits);

    class JsonDDLStructDef
    {
    public:
		std::string name;
		std::vector<JsonDDLMemberDef> members;
    std::optional<int> totalSize;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLStructDef, name, members, totalSize);

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
		std::vector<JsonDDLStructDef> structs;
        std::vector<JsonDDLEnumDef> enums;
        std::optional<int> defSize;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLDef, version, structs, enums, defSize);

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
        std::vector<JsonDDLDef> defs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLRoot, defFiles, defs);
} // namespace T6