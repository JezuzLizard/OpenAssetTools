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
    class DDL
    {
    public:
        // Canonical limits in T7 linker
        static constexpr auto MAX_STRUCTS = 48;
        static constexpr auto MAX_ENUMS = 32;
        static constexpr auto MAX_MEMBERS = 1023;

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
        };

        class Member : public CommonDDLMemberDef
        {
        public:
            DDL::Member(const std::string& name,
                        DDL::Struct* parent)
                : CommonDDLMemberDef(name, parent)
            {
            }

            static const bool IsStandardSize(const ddlPrimitiveTypes_e typeEnum, const int size, const int arraySize)
            {
                if (typeEnum > DDL_FLOAT_TYPE)
                    return false;

                return DDL_TYPE_FEATURES[typeEnum].size == size / arraySize;
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
