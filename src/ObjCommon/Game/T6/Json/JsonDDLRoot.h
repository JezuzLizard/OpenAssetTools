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

  enum ddlPrimitiveTypes_e
  {
    DDL_BYTE_TYPE,
    DDL_SHORT_TYPE,
    DDL_UINT_TYPE,
    DDL_INT_TYPE,
    DDL_FLOAT_TYPE,
    DDL_FIXEDPOINT_TYPE,
    DDL_UINT64_TYPE,
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

  enum ddlTypeFlags_e
  {
    DDL_FLAG_SIGNED = 0x1,
    DDL_FLAG_LIMITS = 0x2,
    DDL_FLAG_BITFIELDS = 0x4,
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
        std::optional<size_t> bits;
        std::optional<size_t> range;
        std::optional<size_t> fixedPrecisionBits;
        std::optional<size_t> fixedMagnitudeBits;
    };

  NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLMemberLimits, bits, range, fixedPrecisionBits, fixedMagnitudeBits);

    class JsonDDLMemberDef
    {
    public:
		std::string name;
        std::string type;
        std::string permission;
        std::optional<size_t> maxCharacters;
		std::optional<size_t> offset;
        std::optional<size_t> structIndex;
		std::optional<size_t> arrayCount;
        std::optional<size_t> enumIndex;
        std::optional<size_t> permissionEnum;
        std::optional<size_t> totalSize;
        std::optional<std::string> enum_;
        std::optional<JsonDDLMemberLimits> limits;
    };

	NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLMemberDef, name, type, permission, maxCharacters, offset, structIndex, arrayCount, enumIndex, permissionEnum, totalSize, enum_, limits);

    class JsonDDLStructDef
    {
    public:
		std::string name;
		std::vector<JsonDDLMemberDef> members;
    std::optional<size_t> totalSize;
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
        std::optional<size_t> defSize;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLDef, version, structs, enums, defSize);

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
        std::vector<JsonDDLDef> defs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION(JsonDDLRoot, defFiles, defs);
    namespace DDL
    {
      class FeatureSupport
      {
      public:
        size_t size;
        size_t flags;
        uint64_t max;
        int64_t min;
      };

      inline const FeatureSupport typeFeatures[] = {
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
           .size = sizeof(float) * CHAR_BIT,
           .flags = DDL_FLAG_SIGNED,
           .max = INT_MAX,
           .min = INT_MIN,
           },
          {
           .size = sizeof(float) * CHAR_BIT,
           .flags = 0,
           .max = 0,
           .min = 0,
          },
          {
           .size = sizeof(uint64_t) * CHAR_BIT,
           .flags = 0,
           .max = UINT64_MAX,
           .min = 0,
          },
      };

    static bool IsMemberStandardSize(const ddlMemberDef_t& ddlMemberDef)
    {
        const auto memberSize = (ddlMemberDef.size / ddlMemberDef.arraySize);
        if (ddlMemberDef.type < DDL_BYTE_TYPE || ddlMemberDef.type > DDL_UINT64_TYPE)
          return false;
        return typeFeatures[ddlMemberDef.type].size == memberSize;
    }
    } // namespace DDL
} // namespace T6