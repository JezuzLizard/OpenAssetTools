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
        int prevDefVersion;
        explicit JsonDumper(std::ostream& stream)
            : m_primaryStream(stream),
              prevDefVersion(0)
        {
        }

#define DDLAssert(expression, ...) \
DDLAssertInternal(expression, #expression, __VA_ARGS__)

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
 
                for (auto& struc : jsonDDLRoot.defs[i].structs)
                {
                    if (struc.name == "root")
                    {
                        jsonDDLRoot.defs[i].inCalculation.resize(jsonDDLRoot.defs[i].structs.size(), false);
                        RecursivelyTraverseStruct(jsonDDLRoot.defs[i], struc);
                    }

                    DDLAssert(struc.refCount, false);

                    for (auto& member : struc.members)
                    {
                        DDLAssert(member.refCount, false);

                        // Only required for actual arrays
                        if (member.arraySize.value_or(1) == 1)
                            member.arraySize.reset();

                        if (struc.name != "root")
                            member.permission.reset();
                    }
                }

                for (const auto& enum_ : jsonDDLRoot.defs[i].enums)
                {
                    DDLAssert(enum_.refCount, false);
                }

                jDef["def"] = jsonDDLRoot.defs[i];

#ifndef DDL_DEBUG //Only dump unneeded data when debugging
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
                    auto& jsonStruc = jDef["def"]["structs"][j];
                    jsonStruc["size"] = jsonDDLRoot.defs[i].structs[j].size.value();
                    jsonStruc["refCount"] = jsonDDLRoot.defs[i].structs[j].refCount;
                    for (auto k = 0u; k < jsonStruc["members"].size(); k++)
                    {
                        jsonStruc["members"][k]["offset"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_offset;
                        jsonStruc["members"][k]["size"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_size;
                        jsonStruc["members"][k]["externalIndex"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_external_index;
                        jsonStruc["members"][k]["enumIndex"] = jsonDDLRoot.defs[i].structs[j].members[k].link.m_enum_index;
                        jsonStruc["members"][k]["refCount"] = jsonDDLRoot.defs[i].structs[j].members[k].refCount;
                    }
                }

                for (auto j = 0u; j < jsonDDLRoot.defs[i].enums.size(); j++)
                {
                    auto& jsonEnum = jDef["def"]["enums"][j];
                    jsonEnum["memberCount"] = jsonDDLRoot.defs[i].enums[j].members.size();
                    jsonEnum["refCount"] = jsonDDLRoot.defs[i].enums[j].refCount;
                }
#endif

                *secondaryAssetFile << std::setw(4) << jDef << "\n";
            }

#ifdef DDL_DEBUG
            for (const auto& def : jsonDDLRoot.defs)
            {
                for (const auto& struc : def.structs)
                {
                    if (struc.refCount)
                        DDLAssert(struc.size.value() <= def.size.value());
                }

                DDLAssert(def.size.value() == def.structs[0].size.value());
            }
#endif

            ordered_json jRoot;

            jRoot["_tool"] = "oat";
            jRoot["_type"] = "ddlRoot";
            jRoot["_game"] = "t6";
            jRoot["_version"] = T6::OAT_DDL_VERSION;
            jRoot["defFiles"] = jsonDDLRoot.defFiles;

            m_primaryStream << std::setw(4) << jRoot << "\n";
        }

        void DDLAssertInternal(const bool expression,
                               const std::string& expressionString,
                               const bool fatalOnOriginal = true,
                               const bool fatalOnRelink = true) const
        {
#ifndef DDL_DEBUG
            return;
#endif

#ifdef DDL_DEBUG_RELINK
            assert(expression || !fatalOnRelink);
#elifdef DDL_DEBUG_ORIGNAL
            if (!expression)
            {
                //if (!expressionString.contains("refCount"))
                    //__debugbreak();
                std::cout << expressionString << "\n";
                if (fatalOnOriginal)
                    assert(false);
            }
#endif
        }

        bool IsLowerCase(const std::string& string)
        {
            return std::all_of(string.begin(),
                               string.end(),
                               [](unsigned char c)
                               {
                                   return std::islower(c);
                               });
        }

        bool CheckName(const std::string& name)
        {
            if (name.empty())
                return false;

            if (!std::isalpha(name[0]))
                return false;

            for (auto i = 1u; i < name.length(); i++)
            {
                if (!std::isalnum(name[i]) && name.substr(i, 1) != "_" && !std::islower(name[i]))
                    return false;
            }

            if (name == "root" || name == "version")
                return true;

            const auto& it = DDL_KEYWORDS.find(name);
            if (it != DDL_KEYWORDS.end())
                return false;

            return true;
        }

        void TraverseMember(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMember) const
        {
            DDLAssert(jDDLMember.permission.value() == jDDLDef.permissionScope);

            auto enumIndex = jDDLMember.link.m_enum_index;
            if (enumIndex >= 0 && enumIndex < jDDLDef.enums.size())
            {
                jDDLDef.enums[enumIndex].refCount++;
            }

            auto structIndex = jDDLMember.link.m_external_index;
            if (structIndex > 0)
            {
                if (jDDLDef.inCalculation.at(structIndex))
                    DDLAssert(false);

                jDDLDef.inCalculation[structIndex] = true;
                // parentDef.m_member_stack.push_back(*this);
                RecursivelyTraverseStruct(jDDLDef, jDDLDef.structs[structIndex]);
                // parentDef.m_member_stack.pop_back();
            }

            jDDLDef.inCalculation.clear();
            jDDLDef.inCalculation.resize(jDDLDef.structs.size(), false);
        }

        void RecursivelyTraverseStruct(const JsonDDLDef& jDDLDef, const JsonDDLStructDef& jDDLStruct) const
        {
            jDDLStruct.refCount++;
            if (jDDLStruct.refCount > 1)
                return;

            for (const auto& member : jDDLStruct.members)
            {
                member.refCount++;
                if (member.refCount > 1)
                    continue;

                if (jDDLStruct.name == "root")
                    jDDLDef.permissionScope = member.permission.value();
                
                TraverseMember(jDDLDef, member);
            }
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
                        DDLAssert(member.link.m_external_index < static_cast<int>(def.structs.size()));

                        if (member.link.m_external_index > 0 && member.link.m_external_index < def.structs.size())
                        {
                            member.type = def.structs[member.link.m_external_index].name;
                            member.link.m_parent_struct = member.type;
                        }
                        else
                            member.link.m_parent_struct = "root";

                        DDLAssert(member.link.m_enum_index == 0 || member.link.m_enum_index < static_cast<int>(def.enums.size()));

                        if (member.link.m_enum_index > 0 && member.link.m_enum_index < def.enums.size())
                        {
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
            DDLAssert(ddlMemberDef.arraySize != 1 || (ddlMemberDef.size % ddlMemberDef.arraySize) == 0);
            // If this happens it means the assertion that serverDelta, and clientDelta are not assigned separately is false
            DDLAssert((ddlMemberDef.rangeLimit == ddlMemberDef.serverDelta) && (ddlMemberDef.rangeLimit == ddlMemberDef.clientDelta));

            auto memberUnitSize = (ddlMemberDef.size / ddlMemberDef.arraySize);

            if (ddlMemberDef.type == DDL_FIXEDPOINT_TYPE)
            {
                DDLAssert(ddlMemberDef.rangeLimit > 0);
                DDLAssert((memberUnitSize - ddlMemberDef.rangeLimit) > 0);

                jDDLMemberLimits.fixedMagnitudeBits.emplace(memberUnitSize - ddlMemberDef.rangeLimit);
                jDDLMemberLimits.fixedPrecisionBits.emplace(ddlMemberDef.rangeLimit);
            }
            else if (ddlMemberDef.rangeLimit != 0)
            {
                DDLAssert(ddlMemberDef.type == DDL_UINT_TYPE || ddlMemberDef.type == DDL_INT_TYPE);
                DDLAssert((ddlMemberDef.type == DDL_UINT_TYPE || memberUnitSize == (std::bit_width(ddlMemberDef.rangeLimit) + 1)));
                DDLAssert((ddlMemberDef.type == DDL_INT_TYPE || memberUnitSize == std::bit_width(ddlMemberDef.rangeLimit)));
                jDDLMemberLimits.range.emplace(ddlMemberDef.rangeLimit);
            }
            else
                jDDLMemberLimits.bits.emplace(memberUnitSize);
        }

        void CreateJsonDDlMemberDef(JsonDDLMemberDef& jDDLMemberDef, const ddlMemberDef_t& ddlMemberDef)
        {
            DDLAssert(CheckName(ddlMemberDef.name));
            DDLAssert(ddlMemberDef.type >= DDL_BYTE_TYPE && ddlMemberDef.type < DDL_TYPE_COUNT);
            DDLAssert(ddlMemberDef.size > 0);
            DDLAssert(ddlMemberDef.arraySize > 0);
            DDLAssert(ddlMemberDef.offset >= 0);
            DDLAssert(ddlMemberDef.externalIndex >= 0 && ddlMemberDef.externalIndex < T6::DDL::MAX_STRUCTS);
            DDLAssert(ddlMemberDef.enumIndex >= -1 && ddlMemberDef.enumIndex < T6::DDL::MAX_ENUMS);
            DDLAssert(ddlMemberDef.type != DDL_STRING_TYPE || (ddlMemberDef.size % CHAR_BIT) == 0);

            JsonDDLMemberLimits jLimits;
            jDDLMemberDef.name = ddlMemberDef.name;
            auto typeEnum = static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type);
            jDDLMemberDef.link.m_type_category = typeEnum;
            jDDLMemberDef.type = T6::DDL::Member::TypeToName(typeEnum);

            DDLAssert(jDDLMemberDef.type != "unknown");

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

            const auto permissionCategory = static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission);

            DDLAssert(permissionCategory >= DDL_PERMISSIONS_CLIENTONLY && permissionCategory < DDL_PERMISSIONS_COUNT);

            jDDLMemberDef.permission.emplace(T6::DDL::Member::PermissionTypeToName(permissionCategory));

            DDLAssert(jDDLMemberDef.permission.has_value() && jDDLMemberDef.permission.value() != "unknown");
        }

        void CreateJsonDDlStructList(JsonDDLStructDef& jDDLStructDef, const ddlStructDef_t& ddlStructDef)
        {
            DDLAssert(CheckName(ddlStructDef.name));
            DDLAssert(ddlStructDef.memberCount > 0 && ddlStructDef.memberCount < T6::DDL::MAX_MEMBERS);
            DDLAssert(ddlStructDef.size > 0);
            DDLAssert(ddlStructDef.members);
            DDLAssert(ddlStructDef.hashTable);

            jDDLStructDef.name = ddlStructDef.name;
            jDDLStructDef.members.resize(ddlStructDef.memberCount);
            jDDLStructDef.size.emplace(ddlStructDef.size);

            auto prevHash = 0;
            auto calculatedStructSize = 0;
            auto prevOffset = 0;
            std::vector<bool> uniqueIndexes;
            uniqueIndexes.resize(ddlStructDef.memberCount, false);

            std::set<std::string> uniqueMembers;

            for (auto i = 0; i < ddlStructDef.memberCount; i++)
            {
                DDLAssert(ddlStructDef.hashTable[i].hash);
                DDLAssert(ddlStructDef.hashTable[i].index >= 0 && ddlStructDef.hashTable[i].index < ddlStructDef.memberCount);

                DDLAssert(prevHash == 0 || ddlStructDef.hashTable[i].hash > prevHash, false); // This is actually cursed
                DDLAssert(!uniqueIndexes[ddlStructDef.hashTable[i].index]);
                DDLAssert(!uniqueMembers.contains(ddlStructDef.members[i].name), false); // HOW?!

                prevHash = ddlStructDef.hashTable[i].hash;
                uniqueIndexes[ddlStructDef.hashTable[i].index] = true;
                uniqueMembers.insert(ddlStructDef.members[i].name);

                DDLAssert(ddlStructDef.members[i].offset <= ddlStructDef.size);
                DDLAssert(ddlStructDef.members[i].offset == 0 || ddlStructDef.members[i].offset > prevOffset);
                DDLAssert(ddlStructDef.members[i].offset == calculatedStructSize);
                DDLAssert(ddlStructDef.members[i].size <= ddlStructDef.size);

                prevOffset = ddlStructDef.members[i].offset;
                calculatedStructSize += ddlStructDef.members[i].size;

                jDDLStructDef.sortedHashTable.push_back(*reinterpret_cast<DDLHashEntry*>(&ddlStructDef.hashTable[i]));
                CreateJsonDDlMemberDef(jDDLStructDef.members[i], ddlStructDef.members[i]);
            }

            DDLAssert(ddlStructDef.size == calculatedStructSize);
        }

        void CreateJsonDDlEnumList(JsonDDLEnumDef& jDDLEnumDef, const ddlEnumDef_t& ddlEnumDef)
        {
            DDLAssert(CheckName(ddlEnumDef.name));
            DDLAssert(ddlEnumDef.memberCount > 0 && ddlEnumDef.memberCount < T6::DDL::MAX_MEMBERS);
            DDLAssert(ddlEnumDef.members && ddlEnumDef.members[0]);
            DDLAssert(ddlEnumDef.hashTable);

            jDDLEnumDef.name = ddlEnumDef.name;
            jDDLEnumDef.members.resize(ddlEnumDef.memberCount);

            auto prevHash = 0;
            std::vector<bool> uniqueIndexes;
            uniqueIndexes.resize(ddlEnumDef.memberCount, false);

            std::set<std::string> uniqueMembers;

            for (auto i = 0; i < ddlEnumDef.memberCount; i++)
            {
                DDLAssert(ddlEnumDef.hashTable[i].hash);
                DDLAssert(ddlEnumDef.hashTable[i].index >= 0 && ddlEnumDef.hashTable[i].index < ddlEnumDef.memberCount);

                DDLAssert(prevHash == 0 || ddlEnumDef.hashTable[i].hash > prevHash, false);
                DDLAssert(!uniqueIndexes[ddlEnumDef.hashTable[i].index]);
                DDLAssert(!uniqueMembers.contains(ddlEnumDef.members[i]), false);

                prevHash = ddlEnumDef.hashTable[i].hash;
                uniqueIndexes[ddlEnumDef.hashTable[i].index] = true;
                uniqueMembers.insert(ddlEnumDef.members[i]);

                DDLAssert(CheckName(ddlEnumDef.members[i]));

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

            DDLAssert(ddlDef->version > 0);
            DDLAssert(prevDefVersion == 0 || prevDefVersion > ddlDef->version);
            DDLAssert(ddlDef->size > 0);
            DDLAssert(ddlDef->structCount > 0 && ddlDef->structCount < T6::DDL::MAX_STRUCTS);
            DDLAssert(ddlDef->enumCount >= 0 && ddlDef->enumCount < T6::DDL::MAX_ENUMS);

            prevDefVersion = ddlDef->version;

            jsonDDLDef.version = ddlDef->version;
            jsonDDLDef.size.emplace(ddlDef->size);

            if (ddlDef->structCount > 0)
            {
                DDLAssert(ddlDef->structList);
                DDLAssert(!strncmp(ddlDef->structList[0].name, "root", sizeof("root")));

                jsonDDLDef.structs.resize(ddlDef->structCount);

                for (auto i = 0; i < ddlDef->structCount; i++)
                {
                    CreateJsonDDlStructList(jsonDDLDef.structs[i], ddlDef->structList[i]);
                }
            }

            if (ddlDef->enumCount > 0)
            {
                DDLAssert(ddlDef->enumList);

                jsonDDLDef.enums.resize(ddlDef->enumCount);
                for (auto i = 0; i < ddlDef->enumCount; i++)
                    CreateJsonDDlEnumList(jsonDDLDef.enums[i], ddlDef->enumList[i]);
            }

            auto filename = baseFilename.stem().string();
            auto extension = ".ddldef.json";
            auto parentFolder = baseFilename.parent_path().string();
            auto filenameFinal = std::format("{}/{}.version_{}{}", parentFolder, filename, jsonDDLDef.version, extension);
            jsonDDLDef.filename = filenameFinal;
            jDDLRoot.defFiles.push_back(filenameFinal);
            jDDLRoot.defs.push_back(jsonDDLDef);

            if (!ddlDef->next)
                return;
            
            CreateJsonDDlDef(jDDLRoot, ddlDef->next, baseFilename, context);
        }

        void CreateJsonDDLRoot(JsonDDLRoot& jDDLRoot, const ddlRoot_t& ddlRoot, AssetDumpingContext& context)
        {
            std::filesystem::path baseFilename = ddlRoot.name;
            prevDefVersion = 0;
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
