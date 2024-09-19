#pragma once

#include "Game/T6/CommonT6.h"

#include "DDL/CommonDDLDef.h"
#include "DDL/CommonDDLEnum.h"
#include "DDL/CommonDDLMember.h"
#include "DDL/CommonDDLStruct.h"
#include "DDL/CommonDDLRoot.h"

#include <algorithm>
#include <string>
#include <cassert>

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

    inline const DDLTypeFeature DDL_TYPE_FEATURES[6] = {
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

    public:
        DDL()
        {

        }
        class Def : public CommonDDLDef
        {
        public:
            Def(const int version, const std::string& filename, CommonDDLRoot& root, const bool isInclude)
                : CommonDDLDef(version, filename, root, isInclude)
            {

            }

            constexpr DDLGameFeatures GetFeatures() const override
            {
                return DDL_GAME_FEATURES;
            }

            CommonDDLDef& Convert(const ddlDef_t* from) const
            {
            }
        };

        class Enum : public CommonDDLEnumDef
        {
        public:
            DDL::Enum(const std::string& name, CommonDDLDef* parent, const size_t index)
                : CommonDDLEnumDef(name, parent, index)
            {

            }

            DDL::Enum(const std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile)
                : CommonDDLEnumDef(name, parent, index, includeFile)
            {
            }
            void CalculateHashes() override
            {
                if (IsCalculated())
                    return;

                for (auto i = 0u; i < m_members.size(); i++)
                {
                    DDLHashEntry ddlHash = {};
                    ddlHash.hash = Common::Com_HashString(m_members[i].c_str());
                    ddlHash.index = i;
                    GetHashTable().emplace_back(ddlHash);
                }

                std::sort(GetHashTable().begin(),
                          GetHashTable().end(),
                          [](const DDLHashEntry& a, const DDLHashEntry& b)
                          {
                              return a.hash < b.hash;
                          });

                SetCalculated();
            }
        };

        class Struct : public CommonDDLStructDef
        {
        public:
            DDL::Struct(std::string& name, CommonDDLDef* parent, const size_t index)
                : CommonDDLStructDef(name, parent, index)
            {
            }

            DDL::Struct(std::string& name, CommonDDLDef* parent, const size_t index, std::string& includeFile)
                : CommonDDLStructDef(name, parent, index, includeFile)
            {
            }
            void CalculateHashes() override
            {
                auto i = 0;
                for (const auto& [k, member] : m_members)
                {
                    DDLHashEntry ddlHash = {};
                    ddlHash.hash = Common::Com_HashString(member.m_name.c_str());
                    ddlHash.index = i;
                    GetHashTable().push_back(ddlHash);
                    i++;
                }

                std::sort(GetHashTable().begin(),
                          GetHashTable().end(),
                          [](const DDLHashEntry& a, const DDLHashEntry& b)
                          {
                              return a.hash < b.hash;
                          });
            }
        };

        class Member : public CommonDDLMemberDef
        {
        public:
            DDL::Member(const std::string& name,
                        DDL::Struct& parent)
                : CommonDDLMemberDef(name, parent)
            {
            }
            const bool IsStandardSize() const override
            {
                if (m_link_data.m_type_category > DDL_FLOAT_TYPE)
                    return false;

                return DDL_TYPE_FEATURES[m_link_data.m_type_category].size == m_link_data.m_size / m_array_size.value_or(1);
            }

            static const bool IsStandardSize(const ddlPrimitiveTypes_e typeEnum, const int size, const int arraySize)
            {
                if (typeEnum > DDL_FLOAT_TYPE)
                    return false;

                return DDL_TYPE_FEATURES[typeEnum].size == size / arraySize;
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

            const uint64_t GetStandardMaxValue() const override
            {
                return DDL_TYPE_FEATURES[m_link_data.m_type_category].max;
            }

            const int64_t GetStandardMinValue() const override
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
                return m_link_data.m_type_category == DDL_FIXEDPOINT_TYPE;
            }

            const bool IsStringType() const override
            {
                return m_link_data.m_type_category == DDL_STRING_TYPE;
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
                return m_link_data.m_type_category < DDL_TYPE_COUNT;
            }

            const bool IsValidPermission() const override
            {
                return m_permission.value_or(DDL_PERMISSIONS_COUNT) < DDL_PERMISSIONS_COUNT;
            }

            const std::string PermissionTypeToName() const override
            {
                if (!m_permission.has_value())
                    return "unknown";

                for (auto& [k, v] : DDL_PERM_NAMES)
                {
                    if (v == m_permission.value())
                        return k;
                }

                return "unknown";
            }

            static const std::string PermissionTypeToName(ddlPermissionTypes_e permission)
            {
                if (permission >= DDL_PERMISSIONS_COUNT)
                    return "unknown";

                for (auto& [k, v] : DDL_PERM_NAMES)
                {
                    if (v == permission)
                        return k;
                }

                return "unknown";
            }

            const std::string TypeToName() const override
            {
                if (!m_type.empty())
                    return m_type;

                if (m_link_data.m_type_category > DDL_TYPE_NAMES.size())
                    return "unknown";

                for (const auto& [k, v] : DDL_TYPE_NAMES)
                {
                    if (v == m_link_data.m_type_category)
                        return k;
                }

                return "unknown";
            }

            static const std::string TypeToName(ddlPrimitiveTypes_e typeEnum)
            {
                if (typeEnum > DDL_TYPE_NAMES.size())
                    return "unknown";

                for (const auto& [k, v] : DDL_TYPE_NAMES)
                {
                    if (v == typeEnum)
                        return k;
                }

                return "unknown";
            }

            const size_t NameToType() const override
            {
                auto it = DDL_TYPE_NAMES.find(m_type);
                if (it != DDL_TYPE_NAMES.end())
                    return static_cast<ddlPrimitiveTypes_e>(it->second);

                return DDL_TYPE_COUNT;
            }

            const int NameToPermissionType(const std::string& typeName) const override
            {
                auto it = DDL_TYPE_NAMES.find(typeName);
                if (it != DDL_TYPE_NAMES.end())
                    return static_cast<ddlPermissionTypes_e>(it->second);

                return DDL_PERMISSIONS_COUNT;
            }
        };

        class Root : public CommonDDLRoot
        {
        public:
            DDL::Root(std::string& name)
                : CommonDDLRoot(name)
            {
            }

            //std::unordered_map<std::string /*m_def_file*/, std::vector<DDL::Def>> m_defs;
            //std::unordered_map<std::string /*m_def_file*/, std::vector<DDL::Def>> m_include_pool;
        };
    };
} // namespace T6
