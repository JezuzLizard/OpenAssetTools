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

    class JsonDDLLoadException : public std::runtime_error
    {
    public:
        JsonDDLLoadException(const std::string& message)
            : std::runtime_error(message)
        {
        }
    };

    class ddlMemberDef_t_ext : ddlMemberDef_t
    {
    public:
        bool IsStandardSize() const;
        std::string PermissionTypeToName() const;
        std::string TypeToName() const;
        ddlPrimitiveTypes_e NameToType(const std::string& typeName) const;
        ddlPermissionTypes_e NameToPermissionType(const std::string& typeName) const;
    };

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
        std::string permission;
        std::optional<size_t> maxCharacters;
		std::optional<size_t> offset;
        std::optional<size_t> structIndex;
		std::optional<size_t> arrayCount;
        std::optional<size_t> enumIndex;
        std::optional<size_t> permissionEnum;
        std::optional<size_t> memberSize;
        std::optional<JsonDDLMemberLimits> limits;

        void Validate() const;
    private: 
        void FixedPoint() const;
        void Range() const;
        void BitFields() const;
    };

	NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(
        JsonDDLMemberDef, name, type, enum_, permission, maxCharacters, offset, structIndex, arrayCount, enumIndex, permissionEnum, memberSize, limits);

    class JsonDDLStructDef
    {
    public:
		std::string name;
    std::optional<size_t> structSize;
		std::vector<JsonDDLMemberDef> members;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLStructDef, name, members, structSize);

    class JsonDDLEnumDef
    {
    public:
		std::string name;
		std::vector<std::string> members;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLEnumDef, name, members);

    class JsonDDLDef
    {
    public:
		int version;
    std::optional<size_t> defSize;
    std::vector<JsonDDLEnumDef> enums;
		std::vector<JsonDDLStructDef> structs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLDef, version, structs, enums, defSize);

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
        std::vector<JsonDDLDef> defs;
    };

    NLOHMANN_DEFINE_TYPE_EXTENSION_ORDERED(JsonDDLRoot, defFiles, defs);

      namespace DDL
      {
          bool IsStandardSize(const size_t size, const ddlPrimitiveTypes_e type, const size_t arraySize = 1)
          {
              const auto memberSize = (size / arraySize);
              if (type < DDL_BYTE_TYPE || type > DDL_UINT64_TYPE)
                  return false;
              return DDL_TYPE_FEATURES[type].size == memberSize;
          }

          std::string PermissionTypeToName(const ddlPermissionTypes_e type)
          {
              if (type <= DDL_PERM_UNSPECIFIED || type >= DDL_PERM_COUNT)
                  return static_cast<int>(type) + "_unknown";

              return DDL_PERM_NAMES[type];
          }

          std::string TypeToName(const ddlPrimitiveTypes_e type)
          {
              if (type >= DDL_TYPE_COUNT)
                  return static_cast<int>(type) + "_unknown";

              return DDL_TYPE_NAMES[type];
          }

          ddlPrimitiveTypes_e NameToType(const std::string& typeName)
          {
              std::string copy(typeName);
              utils::MakeStringLowerCase(copy);

              for (auto i = 0; i < DDL_TYPE_COUNT; i++)
              {
                  if (copy == DDL_TYPE_NAMES[i])
                      return static_cast<ddlPrimitiveTypes_e>(i);
              }

              return DDL_TYPE_COUNT;
          }

          ddlPermissionTypes_e NameToPermissionType(const std::string& typeName)
          {
              std::string copy(typeName);
              utils::MakeStringLowerCase(copy);

              for (auto i = 1; i < DDL_PERM_COUNT; i++)
              {
                  if (copy == DDL_PERM_NAMES[i])
                      return static_cast<ddlPermissionTypes_e>(i);
              }

              return DDL_PERM_UNSPECIFIED;
          }
      } // namespace DDL
} // namespace T6