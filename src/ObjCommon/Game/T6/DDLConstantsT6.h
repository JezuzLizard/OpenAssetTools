#pragma once

#include "Game/T6/CommonT6.h"
#include "DDL/CommonDDLDef.h"

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
                                                      .m_bool_type = false};

    class DDL
    {
        class Def : public CommonDDLDef
        {
            const DDLGameFeatures& GetFeatures() override
            {
                return DDL_GAME_FEATURES;
            }
        };

        class Enum : public CommonDDLEnumDef
        {
            void CalculateHashes() override
            {
                if (IsCalculated())
                    return;

                for (auto i = 0; i < m_members.size(); i++)
                {
                    DDLHash ddlHash = {};
                    ddlHash.hash = Common::Com_HashString(m_members[i].c_str());
                    ddlHash.index = i;
                    GetHashTable().push_back(ddlHash);
                }

                std::sort(GetHashTable().begin(),
                          GetHashTable().end(),
                          [](const DDLHash& a, const DDLHash& b)
                          {
                              return a.hash < b.hash;
                          });

                SetCalculated();
            }
        };

        class Struct : public CommonDDLStructDef
        {
            void CalculateHashes() override
            {
                for (auto i = 0; i < m_members.size(); i++)
                {
                    DDLHash ddlHash = {};
                    ddlHash.hash = Common::Com_HashString(m_members[i].m_name.c_str());
                    ddlHash.index = i;
                    GetHashTable().push_back(ddlHash);
                }

                std::sort(GetHashTable().begin(),
                          GetHashTable().end(),
                          [](const DDLHash& a, const DDLHash& b)
                          {
                              return a.hash < b.hash;
                          });
            }
        };

        class Member : public CommonDDLMemberDef
        {
            const bool IsStandardSize() const override
            {
                if (m_link_data.m_type_enum > DDL_FLOAT_TYPE)
                    return false;

                return DDL_TYPE_FEATURES[m_link_data.m_type_enum].size == m_link_data.m_size / m_array_size.value_or(1);
            }

            const bool IsStandardType() const override
            {
                return m_link_data.m_type_enum <= DDL_FLOAT_TYPE;
            }

            const size_t GetStandardSize() const override
            {
                if (!IsStandardSize())
                    return 0;

                DDL_TYPE_FEATURES[m_link_data.m_type_enum].size;
            }

            const size_t GetStandardMaxValue() const override
            {
                return DDL_TYPE_FEATURES[m_link_data.m_type_enum].max;
            }

            const size_t GetStandardMinValue() const override
            {
                return DDL_TYPE_FEATURES[m_link_data.m_type_enum].min;
            }

            const bool TypeCanUseBitfields() const override
            {
                if (m_link_data.m_type_enum >= DDL_STRING_TYPE)
                    return false;
                return (DDL_TYPE_FEATURES[m_link_data.m_type_enum].flags & DDL_FLAG_BITFIELDS) != 0;
            }

            const bool TypeCanUseLimits() const override
            {
                if (m_link_data.m_type_enum >= DDL_STRING_TYPE)
                    return false;
                return (DDL_TYPE_FEATURES[m_link_data.m_type_enum].flags & DDL_FLAG_LIMITS) != 0;
            }

            const bool TypeCanUseFixedFloatingPoint() const override
            {
                return m_link_data.m_type_enum == DDL_FIXEDPOINT_TYPE;
            }

            const bool IsStringType() const override
            {
                return m_link_data.m_type_enum == DDL_STRING_TYPE;
            }
 
            const size_t GetGameStructType() const override
            {
                return DDL_STRUCT_TYPE;
            };

            const size_t GetGameEnumType() const override
            {
                return DDL_ENUM_TYPE;
            };

            const bool IsValidType() const override
            {
                return m_link_data.m_type_enum < DDL_TYPE_COUNT;
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

                if (m_link_data.m_type_enum < 0 || m_link_data.m_type_enum > DDL_TYPE_NAMES.size())
                    return unknownType;

                for (const auto& [k, v] : DDL_TYPE_NAMES)
                {
                    if (v == m_link_data.m_type_enum)
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

            int NameToPermissionType(const DDLString& typeName) const override
            {
                auto it = DDL_TYPE_NAMES.find(typeName.GetLowerConst());
                if (it != DDL_TYPE_NAMES.end())
                    return static_cast<ddlPermissionTypes_e>(it->second);

                return DDL_PERMISSIONS_COUNT;
            }
        };
    };
} // namespace T6
