#include "JsonDDLRootWriter.h"

#include "Game/T6/DDLConstantsT6.h"
#include "Json/DDL/JsonDDL.h"

#include <iomanip>

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
                jDef["_version"] = T6::OAT_DDL_VERSION;
                jDef["_codeversion"] = 1;
#ifndef DDL_DEBUG //Only dump unneeded data when debugging
                for (auto j = 0u; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto k = 0u; k < jsonDDLRoot.defs[i].structs[j].members.size(); k++)
                    {
                        JsonDDLMemberDef& member = jsonDDLRoot.defs[i].structs[j].members[k];

                        // Only required for actual arrays
                        if (member.arraySize.value_or(1) == 1)
                            member.arraySize.reset();

                        if (member.data.GetParent().m_name != "root")
                            member.permission.reset();
                    }
                }
                jDef["def"] = jsonDDLRoot.defs[i];
#else
                jDef["defSize"] = ddlRoot.ddlDef[i].size;
                for (const auto& struc : jsonDDLRoot.defs[i].structs)
                {
                    for (auto k = 0u; k < struc.sortedHashTable.size(); k++)
                    {
                        DDLHashEntry ddlHash = struc.sortedHashTable.at(k);
                        jDef["sortedHashTables"]["structs"][struc.name][k]["hash"] = ddlHash.hash;
                        jDef["sortedHashTables"]["structs"][struc.name][k]["index"] = ddlHash.index;
                    }
                }

                for (const auto& enum_ : jsonDDLRoot.defs[i].enums)
                {
                    for (auto k = 0u; k < enum_.sortedHashTable.size(); k++)
                    {
                        DDLHashEntry ddlHash = enum_.sortedHashTable.at(k);
                        jDef["sortedHashTables"]["enums"][enum_.name][k]["hash"] = ddlHash.hash;
                        jDef["sortedHashTables"]["enums"][enum_.name][k]["index"] = ddlHash.index;
                    }
                }

                jDef["def"] = jsonDDLRoot.defs[i];
                for (auto j = 0u; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto& jsonStruc : jDef["def"][j])
                    {
                        jsonStruc["size"] = jsonDDLRoot.defs[i].structs[j].size.value();
                        for (auto k = 0u; k < jsonStruc["members"].size(); k++)
                        {
                            jsonStruc["members"][k]["offset"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_offset;
                            jsonStruc["members"][k]["size"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_size;
                            jsonStruc["members"][k]["externalIndex"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_external_index;
                            jsonStruc["members"][k]["enumIndex"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_enum_index;
                        }
                    }
                }
#endif
                *secondaryAssetFile << std::setw(4) << jDef << "\n";
            }
            ordered_json jRoot;

            jRoot["_tool"] = "oat";
            jRoot["_type"] = "ddlRoot";
            jRoot["_game"] = "t6";
            jRoot["_version"] = T6::OAT_DDL_VERSION;
            jRoot["defFiles"] = jsonDDLRoot.defFiles;

            m_primaryStream << std::setw(4) << jRoot << "\n";
        }

        void ResolveCustomTypes(JsonDDLRoot& jDDLRoot)
        {
            for (auto& def : jDDLRoot.defs)
            {
                for (auto& struc : def.structs)
                {
                    const auto& members = struc.members;
                    
                    for (auto& member : struc.members)
                    {
                        if (member.link.m_external_index > 0)
                        {
                            assert(member.link.m_external_index < def.structs.size());

                            member.link.m_struct.emplace(def.structs[member.link.m_external_index].name);
                            member.type = member.link.m_struct.value();
                        }

                        if (member.link.m_enum_index > -1)
                        {
                            assert(member.link.m_enum_index < def.enums.size());

                            member.enum_.emplace(def.enums[member.link.m_enum_index].name);
                        }
                    }

                    struc.size.emplace(members.back().link.m_offset + members.back().link.m_size);
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
            jDDLMemberDef.link.m_type_enum = static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type);
            jDDLMemberDef.type = jDDLMemberDef.data.TypeToName();

            jDDLMemberDef.link.m_size = ddlMemberDef.size;

            jDDLMemberDef.link.m_enum_index = ddlMemberDef.enumIndex;
            jDDLMemberDef.arraySize.emplace(ddlMemberDef.arraySize);

            //.size field has different implications depending on the type
            //string type treat it as max bytes
            //struct is based on the size of the struct
            //enum is based on the type, and also modifies arraySize to the count of its members
            if (ddlMemberDef.type == DDL_STRING_TYPE)
                jDDLMemberDef.maxCharacters.emplace(ddlMemberDef.size / CHAR_BIT);
            else if (ddlMemberDef.type != DDL_STRUCT_TYPE && !jDDLMemberDef.data.IsStandardSize())
                CreateJsonDDlMemberLimits(jDDLMemberDef.limits.emplace(jLimits), ddlMemberDef);

            jDDLMemberDef.link.m_offset = ddlMemberDef.offset;
            
            jDDLMemberDef.link.m_external_index = ddlMemberDef.externalIndex;

            jDDLMemberDef.permission.emplace(static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission));
        }

        void CreateJsonDDlStructList(JsonDDLStructDef& jDDLStructDef, const ddlStructDef_t& ddlStructDef)
        {
            jDDLStructDef.name = ddlStructDef.name;
            jDDLStructDef.members.resize(ddlStructDef.memberCount);

            for (auto i = 0; i < ddlStructDef.memberCount; i++)
            {
                jDDLStructDef.sortedHashTable.push_back(*reinterpret_cast<DDLHashEntry*>(&ddlStructDef.hashTable[i]));
                CreateJsonDDlMemberDef(jDDLStructDef.members[i], ddlStructDef.members[i]);
            }
        }

        void CreateJsonDDlEnumList(JsonDDLEnumDef& jDDLEnumDef, const ddlEnumDef_t& ddlEnumDef)
        {
            jDDLEnumDef.name = ddlEnumDef.name;
            jDDLEnumDef.members.resize(ddlEnumDef.memberCount);

            for (auto i = 0; i < ddlEnumDef.memberCount; i++)
            {
                jDDLEnumDef.sortedHashTable.push_back(*reinterpret_cast<DDLHashEntry*>(&ddlEnumDef.hashTable[i]));
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
