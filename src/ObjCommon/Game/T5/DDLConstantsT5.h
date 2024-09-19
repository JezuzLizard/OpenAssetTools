#pragma once

#include "Game/T5/CommonT5.h"
#include "DDL/CommonDDL.h"
#include <unordered_map>

namespace T5
{
    constexpr auto OAT_DDL_VERSION = 1u;

    enum ddlPrimitiveTypes_e : size_t
    {
        DDL_BYTE_TYPE,
        DDL_SHORT_TYPE,
        DDL_INT_TYPE,
        DDL_UINT64_TYPE,
        DDL_FLOAT_TYPE,
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
        {"byte",   DDL_BYTE_TYPE  },
        {"short",  DDL_SHORT_TYPE },
        {"int",    DDL_INT_TYPE   },
        {"uint64", DDL_UINT64_TYPE},
        {"float",  DDL_FLOAT_TYPE },
        {"string", DDL_STRING_TYPE},
        {"struct", DDL_STRUCT_TYPE},
        {"enum",   DDL_ENUM_TYPE  },
        {"",       DDL_TYPE_COUNT }
    };

    inline const std::unordered_map<std::string, int> DDL_PERM_NAMES = {
        {"unspecified", DDL_PERMISSIONS_UNSPECIFIED},
        {"client",      DDL_PERMISSIONS_CLIENTONLY },
        {"server",      DDL_PERMISSIONS_SERVERONLY },
        {"both",        DDL_PERMISSIONS_BOTH       },
        {"",            DDL_PERMISSIONS_COUNT      }
    };

    inline const DDLTypeFeature DDL_TYPE_FEATURES[] = {
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

    inline const DDLGameFeatures DDL_GAME_FEATURES = {.m_simple_hash_table = false,
                                                      .m_split_hash_table = false,
                                                      .m_header = false,
                                                      .m_padding = false,
                                                      .m_uint_type = false,
                                                      .m_fixed_point_type = false,
                                                      .m_bool_type = false};

    class DDL
    {
    public:
        DDL() {}
        class Def : public CommonDDLDef
        {
        public:
            Def(const int version, const std::string filename)
                : CommonDDLDef(version, filename)
            {
            }

            const DDLGameFeatures& GetFeatures() const override
            {
                return DDL_GAME_FEATURES;
            }
        };

        class Enum : public CommonDDLEnumDef
        {
        public:
            DDL::Enum(CommonDDLDef& parent)
                : CommonDDLEnumDef(parent)
            {
            }
            void CalculateHashes() override
            {
                // T5 doesn't use hashtables
                assert(false);
            }
        };

        class Struct : public CommonDDLStructDef
        {
        public:
            DDL::Struct(CommonDDLDef& parent)
                : CommonDDLStructDef(parent)
            {
            }
            void CalculateHashes() override
            {
                // T5 doesn't use hashtables
                assert(false);
            }
        };

        class Member : public CommonDDLMemberDef
        {
        public:
            DDL::Member(const std::string& name, CommonDDLStructDef& parent)
                : CommonDDLMemberDef(name, parent)
            {
            }
            const bool IsStandardSize() const override
            {
                if (m_link_data.m_type_category > DDL_FLOAT_TYPE)
                    return false;

                return DDL_TYPE_FEATURES[m_link_data.m_type_category].size == m_link_data.m_size / m_array_size.value_or(1);
            }

            const bool IsStandardType() const override
            {
                return m_link_data.m_type_category <= DDL_FLOAT_TYPE;
            }

            const size_t GetStandardSize() const override
            {
                if (!IsStandardSize())
                    return 0;

                DDL_TYPE_FEATURES[m_link_data.m_type_category].size;
            }

            const size_t GetStandardMaxValue() const override
            {
                return DDL_TYPE_FEATURES[m_link_data.m_type_category].max;
            }

            const size_t GetStandardMinValue() const override
            {
                return DDL_TYPE_FEATURES[m_link_data.m_type_category].min;
            }

            const bool TypeCanUseBitfields() const override
            {
                if (m_link_data.m_type_category >= DDL_STRING_TYPE)
                    return false;
                return (DDL_TYPE_FEATURES[m_link_data.m_type_category].flags & DDL_FLAG_BITFIELDS) != 0;
            }

            const bool TypeCanUseLimits() const override
            {
                if (m_link_data.m_type_category >= DDL_STRING_TYPE)
                    return false;
                return (DDL_TYPE_FEATURES[m_link_data.m_type_category].flags & DDL_FLAG_LIMITS) != 0;
            }

            const bool TypeCanUseFixedFloatingPoint() const override
            {
                return false;
            }

            const bool IsStringType() const override
            {
                return m_link_data.m_type_category == DDL_STRING_TYPE;
            }

            constexpr size_t GetGameStructType() const override
            {
                return DDL_STRUCT_TYPE;
            };

            constexpr size_t GetGameEnumType() const override
            {
                return DDL_ENUM_TYPE;
            };

            const bool IsValidType() const override
            {
                return m_link_data.m_type_category < DDL_TYPE_COUNT;
            }

            const bool IsValidPermission() const override
            {
                return m_permission.value_or(DDL_PERMISSIONS_COUNT) < DDL_PERMISSIONS_COUNT;
            }

            const std::string& PermissionTypeToName() const override
            {
                static const std::string unknownPerm = "count";

                if (!m_permission.has_value())
                    return unknownPerm;

                for (auto& [k, v] : DDL_PERM_NAMES)
                {
                    if (v == m_permission.value())
                        return k;
                }

                return unknownPerm;
            }

            const std::string& TypeToName() const override
            {
                static const std::string unknownType = "unknown";

                if (m_link_data.m_type_category < 0 || m_link_data.m_type_category > DDL_TYPE_NAMES.size())
                    return unknownType;

                for (const auto& [k, v] : DDL_TYPE_NAMES)
                {
                    if (v == m_link_data.m_type_category)
                        return k;
                }

                return unknownType;
            }

            size_t NameToType() const override
            {
                auto it = DDL_TYPE_NAMES.find(m_type.GetLowerConst());
                if (it != DDL_TYPE_NAMES.end())
                    return static_cast<ddlPrimitiveTypes_e>(it->second);

                return DDL_TYPE_COUNT;
            }

            int NameToPermissionType(const std::string& typeName) const override
            {
                auto it = DDL_TYPE_NAMES.find(typeName.GetLowerConst());
                if (it != DDL_TYPE_NAMES.end())
                    return static_cast<ddlPermissionTypes_e>(it->second);

                return DDL_PERMISSIONS_COUNT;
            }
        };
    };
} // namespace T5
