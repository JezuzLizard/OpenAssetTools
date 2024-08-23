#include "JsonDDLRootWriter.h"

#include "Game/T6/CommonT6.h"
#include "Game/T6/Json/JsonDDLRoot.h"

#include <iomanip>
#include <nlohmann/json.hpp>

using namespace nlohmann;
using namespace T6;

namespace
{
    class JsonDumper
    {
    public:
        explicit JsonDumper(std::ostream& stream)
            : m_primaryStream(stream)
        {
        }

        void Dump(const ddlRoot_t& ddlRoot, AssetDumpingContext& context) const
        {
            JsonDDLRoot jsonDDLRoot;
            CreateJsonDDLRoot(jsonDDLRoot, ddlRoot, context);
            Resolver(jsonDDLRoot);

            for (auto i = 0u; i < jsonDDLRoot.defs.size(); i++)
            {
                const auto secondaryAssetFile = context.OpenAssetFile(jsonDDLRoot.defFiles[i]);
                json jDef;

                jDef["_type"] = "ddlDef";
                jDef["_game"] = "t6";
                jDef["_version"] = 1;
#ifndef DDL_DEBUG //Only dump unneeded data when debugging
                for (auto j = 0; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto k = 0; k < jsonDDLRoot.defs[i].structs[j].members.size(); k++)
                    {
                        JsonDDLMemberDef& member = jsonDDLRoot.defs[i].structs[j].members[k];
                        member.enumIndex.reset();
                        member.offset.reset();
                        member.permissionEnum.reset();
                        member.structIndex.reset();
                        member.totalSize.reset();
                        if (member.structIndex > 0 || member.enumIndex > 0)
                        {
                            member.limits.reset();
                        }
                    }
                }
#endif
                jDef["def"] = jsonDDLRoot.defs[i];

                *secondaryAssetFile << std::setw(4) << jDef << "\n";
            }
            json jRoot;

            jRoot["_type"] = "ddlRoot";
            jRoot["_game"] = "t6";
            jRoot["_version"] = 1;
            jRoot["defFiles"] = jsonDDLRoot.defFiles;

            m_primaryStream << std::setw(4) << jRoot << "\n";
        }

    private:
        static const char* AssetName(const char* input)
        {
            if (input && input[0] == ',')
                return &input[1];

            return input;
        }

        static std::string TypeToName(const ddlPrimitiveTypes_e& type)
        {
            if (type <= DDL_INVALID_TYPE || type >= DDL_TYPE_COUNT)
                return static_cast<int>(type) + "_unknown";

            return DDL_TYPE_NAMES[type];
        }

        static ddlPrimitiveTypes_e NameToType(const std::string& typeName)
        {
            for (auto i = 0; i < DDL_TYPE_COUNT; i++)
            {
                if (typeName == DDL_TYPE_NAMES[i])
                    return static_cast<ddlPrimitiveTypes_e>(i);
            }

            return DDL_INVALID_TYPE;
        }

        static std::string PermissionTypeToName(const ddlPermissionTypes_e& type)
        {
            if (type <= DDL_PERM_UNSPECIFIED || type >= DDL_PERM_COUNT)
                return static_cast<int>(type) + "_unknown";

            return DDL_PERM_NAMES[type];
        }

        static ddlPermissionTypes_e NameToPermissionType(const std::string& typeName)
        {
            for (auto i = 1; i < DDL_PERM_COUNT; i++)
            {
                if (typeName == DDL_PERM_NAMES[i])
                    return static_cast<ddlPermissionTypes_e>(i);
            }

            return DDL_PERM_UNSPECIFIED;
        }

        static std::string ExternalIndexToName(const JsonDDLDef& jDDLDef, const int& index, bool isStruct)
        {
            if (isStruct)
                return index < jDDLDef.structs.size() ? jDDLDef.structs.at(index).name : "couldn't resolve struct name";
            else
                return index < jDDLDef.enums.size() ? jDDLDef.enums.at(index).name : "couldn't resolve enum name";
        }

        static bool IsMemberStandardSize(const ddlMemberDef_t& ddlMemberDef)
        {
            const auto memberSize = (ddlMemberDef.size / ddlMemberDef.arraySize);
            switch (ddlMemberDef.type)
            {
            case DDL_BYTE_TYPE:
                if (memberSize == sizeof(char) * CHAR_BIT)
                    return true;
                break;
            case DDL_SHORT_TYPE:
                if (memberSize == sizeof(short) * CHAR_BIT)
                    return true;
                break;
            case DDL_UINT_TYPE:
            case DDL_INT_TYPE:
            case DDL_FLOAT_TYPE:
                if (memberSize == sizeof(int) * CHAR_BIT)
                    return true;
                break;
            case DDL_UINT64_TYPE:
                if (memberSize == sizeof(long long) * CHAR_BIT)
                    return true;
                break;
            }

            return false;
        }

        static void Resolver(JsonDDLRoot& jDDLRoot)
        {
            for (auto i = 0u; i < jDDLRoot.defs.size(); i++)
            {
                for (auto j = 0; j < jDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto k = 0; k < jDDLRoot.defs[i].structs[j].members.size(); k++)
                    {
                        JsonDDLMemberDef& member = jDDLRoot.defs[i].structs[j].members[k];
                        if (member.type == "struct")
                        {
                            member.type = jDDLRoot.defs[i].structs[member.structIndex.value()].name;
                        }
                        else if (member.type == "enum")
                        {
                            member.type = jDDLRoot.defs[i].enums[member.enumIndex.value()].name;
                        }
                    }
                }
            }
        }

        static void CreateJsonDDlMemberLimits(JsonDDLMemberLimits& jDDLMemberLimits, const ddlMemberDef_t& ddlMemberDef)
        {
            auto memberSize = (ddlMemberDef.size / ddlMemberDef.arraySize);
            if (ddlMemberDef.type == DDL_FIXEDPOINT_TYPE)
            {
                jDDLMemberLimits.fixedMagnitudeBits.emplace(memberSize - ddlMemberDef.rangeLimit);
                jDDLMemberLimits.fixedPrecisionBits.emplace(ddlMemberDef.rangeLimit);
            }
            else if (ddlMemberDef.rangeLimit != 0)
                jDDLMemberLimits.range.emplace(ddlMemberDef.rangeLimit);
            else
                jDDLMemberLimits.bits.emplace(memberSize);
            
            assert((ddlMemberDef.rangeLimit == ddlMemberDef.serverDelta) && (ddlMemberDef.rangeLimit == ddlMemberDef.clientDelta));
        }

        static void CreateJsonDDlMemberDef(JsonDDLMemberDef& jDDLMemberDef, const ddlMemberDef_t& ddlMemberDef)
        {
            JsonDDLMemberLimits jLimits;
            jDDLMemberDef.name = ddlMemberDef.name;
            jDDLMemberDef.type = TypeToName(static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type));

            //.size field has different implications depending on the type
            //string type treat it as maxchars
            //struct is based on the size of the struct
            //enum is based on
            if (ddlMemberDef.type == DDL_STRING_TYPE)
                jDDLMemberDef.maxCharacters.emplace(ddlMemberDef.size);
            else if (ddlMemberDef.type != DDL_ENUM_TYPE && ddlMemberDef.type != DDL_STRUCT_TYPE && !IsMemberStandardSize(ddlMemberDef))
                CreateJsonDDlMemberLimits(jDDLMemberDef.limits.emplace(jLimits), ddlMemberDef);

            jDDLMemberDef.offset.emplace(ddlMemberDef.offset);
            
            if (ddlMemberDef.externalIndex > 0)
                jDDLMemberDef.structIndex.emplace(ddlMemberDef.externalIndex);

            if (ddlMemberDef.arraySize > 1) //Only required for actual arrays
                jDDLMemberDef.arrayCount.emplace(ddlMemberDef.arraySize);
            if (ddlMemberDef.enumIndex > 0)
                jDDLMemberDef.enumIndex.emplace(ddlMemberDef.enumIndex);
            jDDLMemberDef.permission = PermissionTypeToName(static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission));
            jDDLMemberDef.totalSize.emplace(ddlMemberDef.size);
        }

        static void CreateJsonDDlStructList(JsonDDLStructDef& jDDLStructDef, const ddlStructDef_t& ddlStructDef)
        {
            jDDLStructDef.name = ddlStructDef.name;
            jDDLStructDef.members.resize(ddlStructDef.memberCount);

            for (auto i = 0; i < ddlStructDef.memberCount; i++)
                CreateJsonDDlMemberDef(jDDLStructDef.members[i], ddlStructDef.members[i]);
        }

        static void CreateJsonDDlEnumList(JsonDDLEnumDef& jDDLEnumDef, const ddlEnumDef_t& ddlEnumDef)
        {
            jDDLEnumDef.name = ddlEnumDef.name;
            jDDLEnumDef.members.resize(ddlEnumDef.memberCount);

            for (auto i = 0; i < ddlEnumDef.memberCount; i++)
                jDDLEnumDef.members[i] = ddlEnumDef.members[i];
        }

        static void CreateJsonDDlDef(JsonDDLRoot& jDDLRoot,
                                     const ddlDef_t* ddlDef,
                                     const std::filesystem::path baseFilename,
                                     AssetDumpingContext& context)
        {
            JsonDDLDef jsonDDLDef;
            jsonDDLDef.version = ddlDef->version;

            if (ddlDef->structCount > 0)
            {
                jsonDDLDef.structs.resize(ddlDef->structCount);
                for (auto i = 0; i < ddlDef->structCount; i++)
                    CreateJsonDDlStructList(jsonDDLDef.structs[i], ddlDef->structList[i]);
            }

            if (ddlDef->enumCount > 0)
            {
                jsonDDLDef.enums.resize(ddlDef->enumCount);
                for (auto i = 0; i < ddlDef->enumCount; i++)
                    CreateJsonDDlEnumList(jsonDDLDef.enums[i], ddlDef->enumList[i]);
            }

            auto filename = baseFilename.stem().string();
            auto extension = ".ddldef.json";
            auto parentFolder = baseFilename.parent_path().string();
            auto filenameFinal = std::format("{}/{}.version_{}{}", parentFolder, filename, jsonDDLDef.version, extension);
            jDDLRoot.defFiles.push_back(filenameFinal);
            jDDLRoot.defs.push_back(jsonDDLDef);

            if (!ddlDef->next)
                return;
            
            CreateJsonDDlDef(jDDLRoot, ddlDef->next, baseFilename, context);
        }

        static void CreateJsonDDLRoot(JsonDDLRoot& jDDLRoot, const ddlRoot_t& ddlRoot, AssetDumpingContext& context)
        {
            std::filesystem::path baseFilename = ddlRoot.name;
            CreateJsonDDlDef(jDDLRoot, ddlRoot.ddlDef, baseFilename, context);
        }

        std::ostream& m_primaryStream;
    };
} // namespace

namespace T6
{
    void DumpDDLRootAsJson(std::ostream& primaryStream, AssetDumpingContext& context, const ddlRoot_t* ddlRoot)
    {
        const JsonDumper dumper(primaryStream);
        dumper.Dump(*ddlRoot, context);
    }
} // namespace T6
