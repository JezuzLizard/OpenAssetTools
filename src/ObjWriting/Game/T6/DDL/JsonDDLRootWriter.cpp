#include "JsonDDLRootWriter.h"

#include "Game/T6/CommonT6.h"
#include "Game/T6/Json/DDL/JsonDDLRoot.h"

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

        void Dump(const ddlRoot_t& ddlRoot, AssetDumpingContext& context)
        {
            if (!ddlRoot.ddlDef)
            {
                std::cerr << "Tried to dump \"" << ddlRoot.name << "\" but it had no defs";
                return;
            }
            JsonDDLRoot jsonDDLRoot;
            CreateJsonDDLRoot(jsonDDLRoot, ddlRoot, context);
            ResolveCustomTypes(jsonDDLRoot);

            for (auto i = 0u; i < jsonDDLRoot.defs.size(); i++)
            {
                const auto secondaryAssetFile = context.OpenAssetFile(jsonDDLRoot.defFiles[i]);
                ordered_json jDef;

                jDef["_tool"] = "oat";
                jDef["_type"] = "ddlDef";
                jDef["_game"] = "t6";
                jDef["_version"] = OAT_DDL_VERSION;
                jDef["_codeversion"] = 1;
#ifndef DDL_DEBUG //Only dump unneeded data when debugging
                for (auto j = 0u; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto k = 0u; k < jsonDDLRoot.defs[i].structs[j].members.size(); k++)
                    {
                        JsonDDLMemberDef& member = jsonDDLRoot.defs[i].structs[j].members[k];

                        member.linkData.offset.reset();
                        member.linkData.size.reset();
                        if (member.linkData.externalIndex.value_or(-1) > 0)
                        {
                            member.limits.reset();
                        }

                        member.linkData.struct_.reset();

                        // Only required for actual arrays
                        if (member.linkData.enumIndex.value_or(-1) < 0)
                            member.linkData.enumIndex.reset();
                        if (member.arraySize.value_or(1) == 1)
                            member.arraySize.reset();

                        member.linkData.externalIndex.reset();
                    }

                    jsonDDLRoot.defs[i].structs[j].size.reset();
                }
#else
                jDef["defSize"] = ddlRoot.ddlDef[i].size;
                for (auto j = 0u; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto k = 0u; k < jsonDDLRoot.defs[i].structs[j].sortedHashTable.size(); k++)
                    {
                        ddlHash_t ddlHash = jsonDDLRoot.defs[i].structs[j].sortedHashTable.at(k);
                        jDef["sortedHashTables"]["structs"][jsonDDLRoot.defs[i].structs[j].name][k]["hash"] = ddlHash.hash;
                        jDef["sortedHashTables"]["structs"][jsonDDLRoot.defs[i].structs[j].name][k]["index"] = ddlHash.index;
                    }
                }

                for (auto j = 0u; j < jsonDDLRoot.defs[i].enums.size(); j++)
                {
                    for (auto k = 0u; k < jsonDDLRoot.defs[i].enums[j].sortedHashTable.size(); k++)
                    {
                        ddlHash_t ddlHash = jsonDDLRoot.defs[i].enums[j].sortedHashTable.at(k);
                        jDef["sortedHashTables"]["enums"][jsonDDLRoot.defs[i].enums[j].name][k]["hash"] = ddlHash.hash;
                        jDef["sortedHashTables"]["enums"][jsonDDLRoot.defs[i].enums[j].name][k]["index"] = ddlHash.index;
                    }
                }
#endif
                jDef["def"] = jsonDDLRoot.defs[i];
                *secondaryAssetFile << std::setw(4) << jDef << "\n";
            }
            ordered_json jRoot;

            jRoot["_type"] = "ddlRoot";
            jRoot["_game"] = "t6";
            jRoot["_version"] = OAT_DDL_VERSION;
            jRoot["defFiles"] = jsonDDLRoot.defFiles;

            m_primaryStream << std::setw(4) << jRoot << "\n";
        }

        void ResolveCustomTypes(JsonDDLRoot& jDDLRoot)
        {
            for (auto i = 0u; i < jDDLRoot.defs.size(); i++)
            {
                for (auto j = 0u; j < jDDLRoot.defs[i].structs.size(); j++)
                {
                    const auto& members = jDDLRoot.defs[i].structs[j].members;
                    
                    for (auto k = 0u; k < jDDLRoot.defs.at(i).structs[j].members.size(); k++)
                    {
                        JsonDDLMemberDef& member = jDDLRoot.defs[i].structs[j].members[k];
                        if (member.linkData->externalIndex > 0)
                        {
                            member.linkData->struct_.emplace(jDDLRoot.defs[i].structs[member.linkData->externalIndex].name);
                            member.type = member.linkData->struct_.value();
                        }

                        if (member.linkData->enumIndex > -1)
                            member.enum_.emplace(jDDLRoot.defs[i].enums[member.linkData->enumIndex].name);
                    }

                    jDDLRoot.defs[i].structs[j].size.emplace(members.back().linkData->offset + members.back().linkData->size);
                }
            }
        }

        void CreateJsonDDlMemberLimits(JsonDDLMemberLimits& jDDLMemberLimits, const ddlMemberDef_t& ddlMemberDef)
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
            
            //If this happens it means the assertion that serverDelta, and clientDelta are not assigned separately is false
            assert((ddlMemberDef.rangeLimit == ddlMemberDef.serverDelta) && (ddlMemberDef.rangeLimit == ddlMemberDef.clientDelta));
        }

        void CreateJsonDDlMemberDef(JsonDDLMemberDef& jDDLMemberDef, const ddlMemberDef_t& ddlMemberDef)
        {
            JsonDDLMemberLimits jLimits;
            jDDLMemberDef.name = ddlMemberDef.name;
            jDDLMemberDef.linkData->typeEnum = static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type);
            jDDLMemberDef.type = jDDLMemberDef.TypeToName();

            jDDLMemberDef.linkData->size = ddlMemberDef.size;

            jDDLMemberDef.linkData->enumIndex = ddlMemberDef.enumIndex;
            jDDLMemberDef.arraySize.emplace(ddlMemberDef.arraySize);

            //.size field has different implications depending on the type
            //string type treat it as max bytes
            //struct is based on the size of the struct
            //enum is based on the type, and also modifies arraySize to the count of its members
            if (ddlMemberDef.type == DDL_STRING_TYPE)
                jDDLMemberDef.maxCharacters.emplace(ddlMemberDef.size / CHAR_BIT);
            else if (ddlMemberDef.type != DDL_STRUCT_TYPE && !jDDLMemberDef.IsStandardSize())
                CreateJsonDDlMemberLimits(jDDLMemberDef.limits.emplace(jLimits), ddlMemberDef);

            jDDLMemberDef.linkData->offset = ddlMemberDef.offset;
            
            jDDLMemberDef.linkData->externalIndex = ddlMemberDef.externalIndex;

            jDDLMemberDef.permission.emplace(static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission));
        }

        void CreateJsonDDlStructList(JsonDDLStructDef& jDDLStructDef, const ddlStructDef_t& ddlStructDef)
        {
            jDDLStructDef.name = ddlStructDef.name;
            jDDLStructDef.members.resize(ddlStructDef.memberCount);

            for (auto i = 0; i < ddlStructDef.memberCount; i++)
            {
                jDDLStructDef.sortedHashTable.push_back(ddlStructDef.hashTable[i]);
                CreateJsonDDlMemberDef(jDDLStructDef.members[i], ddlStructDef.members[i]);
            }
        }

        void CreateJsonDDlEnumList(JsonDDLEnumDef& jDDLEnumDef, const ddlEnumDef_t& ddlEnumDef)
        {
            jDDLEnumDef.name = ddlEnumDef.name;
            jDDLEnumDef.members.resize(ddlEnumDef.memberCount);

            for (auto i = 0; i < ddlEnumDef.memberCount; i++)
            {
                jDDLEnumDef.sortedHashTable.push_back(ddlEnumDef.hashTable[i]);
                jDDLEnumDef.members[i] = ddlEnumDef.members[i];
            }
        }

        void CreateJsonDDlDef(JsonDDLRoot& jDDLRoot,
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

        void CreateJsonDDLRoot(JsonDDLRoot& jDDLRoot, const ddlRoot_t& ddlRoot, AssetDumpingContext& context)
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
        JsonDumper dumper(primaryStream);
        dumper.Dump(*ddlRoot, context);
    }
} // namespace T6
