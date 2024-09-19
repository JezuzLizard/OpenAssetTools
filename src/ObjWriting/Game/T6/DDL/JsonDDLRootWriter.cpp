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

        void TraverseMember(const JsonDDLMemberDef& jDDLMember, std::string permissionScope)
        {
            assert(jDDLMember.permission.value() == permissionScope);
            const auto& parentStruct = jDDLMember.link.;
            const auto& parentDef = parentStruct.GetParent();
            if (HasEnum())
            {
                auto& enumDef = parentDef.m_enums.at(m_name);
                enumDef.Validate();
            }

            auto index = parentDef.TypeToStructIndex(m_type);
            if (index)
            {
                if (parentDef.m_in_calculation.at(index))
                    ReportCircularDependency("circular dependency detected");

                const auto& structDef = parentDef.m_structs.at(m_name);
                parentDef.m_in_calculation[index] = true;
                parentDef.m_member_stack.push_back(*this);
                structDef.Validate();
                parentDef.m_member_stack.pop_back();
            }

            m_calculated = true;
            parentDef.m_in_calculation.clear();
            parentDef.m_in_calculation.resize(parentDef.m_structs.size(), false);
        }

        void RecursivelyTraverseStruct(const JsonDDLStructDef& jDDLStruct, std::optional<std::string> permissionScope)
        {
            for (const auto& member : jDDLStruct.members)
            {
                member.refCount++;
                if (member.refCount > 0)
                    continue;

                TraverseMember(member, jDDLStruct.name == "root" ? member.permission.value() : "unspecified");
            }
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
#ifdef DDL_DEBUG //Only dump unneeded data when debugging
                for (auto j = 0u; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    if (jsonDDLRoot.defs[i].structs[j].name == "root")
                    {
                        RecursivelyTraverseStruct(jsonDDLRoot.defs[i].structs[j]);
                        break;
                    }
                }
                
                for (auto j = 0u; j < jsonDDLRoot.defs[i].structs.size(); j++)
                {
                    for (auto k = 0u; k < jsonDDLRoot.defs[i].structs[j].members.size(); k++)
                    {
                        JsonDDLMemberDef& member = jsonDDLRoot.defs[i].structs[j].members[k];

                        // Only required for actual arrays
                        if (member.arraySize.value_or(1) == 1)
                            member.arraySize.reset();

                        if (jsonDDLRoot.defs[i].structs[j].name != "root")
                        {
                            member.permission.reset();
                        }
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
                    auto& members = struc.members;
                    
                    for (auto& member : members)
                    {
                        if (member.link.m_external_index > 0)
                        {
                            assert(member.link.m_external_index < def.structs.size());

                            member.type = def.structs[member.link.m_external_index].name;
                            member.link.m_parent_struct = member.type;
                        }
                        else
                            member.link.m_parent_struct = "root";

                        if (member.link.m_enum_index > 0)
                        {
                            assert(member.link.m_enum_index < def.enums.size());

                            member.enum_.emplace(def.enums[member.link.m_enum_index].name);
                        }
                        else
                        {
                            //-1 enumindex indicates no enum
                            //0 enumindex is a reserved value, but is not the same as -1
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

        void CreateJsonDDlMemberDef(JsonDDLMemberDef& jDDLMemberDef, const ddlMemberDef_t& ddlMemberDef, JsonDDLStructDef& jDDLStructDef)
        {
            JsonDDLMemberLimits jLimits;
            jDDLMemberDef.name = ddlMemberDef.name;
            auto typeEnum = static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type);
            jDDLMemberDef.link.m_type_enum = typeEnum;
            jDDLMemberDef.type = T6::DDL::Member::TypeToName(typeEnum);

            jDDLMemberDef.link.m_size = ddlMemberDef.size;

            jDDLMemberDef.link.m_enum_index = ddlMemberDef.enumIndex;
            jDDLMemberDef.arraySize.emplace(ddlMemberDef.arraySize);

            //.size field has different implications depending on the type
            //string type treat it as max bytes
            //struct is based on the size of the struct
            //enum is based on the type, and also modifies arraySize to the count of its members
            if (ddlMemberDef.type == DDL_STRING_TYPE)
                jDDLMemberDef.maxCharacters.emplace(ddlMemberDef.size / CHAR_BIT);
            else if (ddlMemberDef.type != DDL_STRUCT_TYPE && !T6::DDL::Member::IsStandardSize(typeEnum, ddlMemberDef.size, ddlMemberDef.arraySize))
                CreateJsonDDlMemberLimits(jDDLMemberDef.limits.emplace(jLimits), ddlMemberDef);

            jDDLMemberDef.link.m_offset = ddlMemberDef.offset;
            
            jDDLMemberDef.link.m_external_index = ddlMemberDef.externalIndex;

            jDDLMemberDef.permission.emplace(T6::DDL::Member::PermissionTypeToName(static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission)));

            if (jDDLStructDef.name == "root")
                jDDLStructDef.permissionScope.push_back(jDDLMemberDef.permission.value());
        }

        void CreateJsonDDlStructList(JsonDDLStructDef& jDDLStructDef, const ddlStructDef_t& ddlStructDef)
        {
            jDDLStructDef.name = ddlStructDef.name;
            jDDLStructDef.members.resize(ddlStructDef.memberCount);

            for (auto i = 0; i < ddlStructDef.memberCount; i++)
            {
                jDDLStructDef.sortedHashTable.push_back(*reinterpret_cast<DDLHashEntry*>(&ddlStructDef.hashTable[i]));
                CreateJsonDDlMemberDef(jDDLStructDef.members[i], ddlStructDef.members[i], jDDLStructDef);
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
