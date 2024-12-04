#include "JsonDDLRootWriter.h"

#include "Game/T6/DDLConstantsT6.h"
#include "Json/DDL/JsonDDL.h"
#include "Dumping/AbstractTextDumper.h"

#include <iomanip>

using namespace nlohmann;
using namespace T6;

#define DDL_DEBUG 1
//#undef DDL_DEBUG
//#define DDL_MINIMAL_DUMP 1
//#define DDL_DEBUG_RELINK 1

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
                jDef["_version"] = OAT_DDL_VERSION;
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

                        DDLAssert(member.link.m_type_category != DDL_STRUCT_TYPE
                                  || member.link.m_size / member.arraySize.value_or(1) == jsonDDLRoot.defs[i].structs[member.link.m_external_index].size);

                        // Only required for actual arrays, enums arraysize is always derived by the number of members in the enum
                        if (member.arraySize.value_or(1) == 1 || member.enum_.has_value())
                            member.arraySize.reset();

                        if (struc.name != "root")
                            member.permission.reset();
                    }
                }

                for (const auto& enum_ : jsonDDLRoot.defs[i].enums)
                {
                    DDLAssert(enum_.refCount, false);
                }

#ifdef DDL_MINIMAL_DUMP
                auto enumsCopy = jsonDDLRoot.defs[i].enums;
                jsonDDLRoot.defs[i].enums.clear();
                for (auto j = 0; j < enumsCopy.size(); j++)
                {
                    if (enumsCopy[j].refCount != 0)
                    {
                        jsonDDLRoot.defs[i].enums.push_back(enumsCopy[j]);
                    }
                }

                auto structsCopy = jsonDDLRoot.defs[i].structs;
                jsonDDLRoot.defs[i].structs.clear();
                for (auto j = 0; j < structsCopy.size(); j++)
                {
                    if (structsCopy[j].refCount != 0)
                    {
                        jsonDDLRoot.defs[i].structs.push_back(structsCopy[j]);
                    }
                }
                
                // fix up enum/struct index references

                for (auto& struc : jsonDDLRoot.defs[i].structs)
                {
                    for (auto& member : struc.members)
                    {
                        if (member.link.m_external_index >= jsonDDLRoot.defs[i].structs.size())
                        {
                            for (auto j = 0; j < jsonDDLRoot.defs[i].structs.size(); j++)
                            {
                                if (member.type == jsonDDLRoot.defs[i].structs[j].name)
                                {
                                    member.link.m_external_index = j;
                                    break;
                                }
                            }
                        }
                        else if (member.enum_.has_value() && member.link.m_enum_index >= jsonDDLRoot.defs[i].enums.size())
                        {
                            for (auto j = 0; j < jsonDDLRoot.defs[i].enums.size(); j++)
                            {
                                if (member.enum_.value() == jsonDDLRoot.defs[i].enums[j].name)
                                {
                                    member.link.m_enum_index = j;
                                    break;
                                }
                            }
                        }
                    }
                }
#endif

                jDef["def"] = jsonDDLRoot.defs[i];

#ifdef DDL_DEBUG //Only dump unneeded data when debugging
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

            for (auto i = 0; i < jsonDDLRoot.defFiles.size(); i++)
            {
                std::filesystem::path baseFilename = jsonDDLRoot.defFiles[i];
                auto filename = baseFilename.stem().string();
                auto parentFolder = baseFilename.parent_path().string();
                auto filenameFinal = std::format("canon/{}/{}", parentFolder, filename);
                auto canonicalFile = context.OpenAssetFile(filenameFinal);
                DDLDumper dumper(*canonicalFile);
                dumper.DumpDef(jsonDDLRoot.defs[i]);
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
            jRoot["_version"] = OAT_DDL_VERSION;
            jRoot["defFiles"] = jsonDDLRoot.defFiles;

            m_primaryStream << std::setw(4) << jRoot << "\n";
        }

        class DDLDumper : AbstractTextDumper
        {
        public:
            DDLDumper(std::ostream& stream)
                : AbstractTextDumper(stream)
            {
            }
            void WriteLineComment(const std::string& comment) const
            {
                Indent();
                m_stream << "// " << comment << "\n";
            }

            void DumpEnum(const JsonDDLEnumDef& _enum)
            {
                Indent();

                m_stream << "enum ";

                m_stream << _enum.name << "\n";

                Indent();
                m_stream << "{\n";

                IncIndent();

                const auto entryCount = _enum.members.size();
                for (auto i = 0u; i < entryCount; i++)
                {
                    Indent();
                    m_stream << _enum.members[i];
                    m_stream << "\n";
                }

                DecIndent();
                Indent();
                m_stream << "};\n"; // end enum
            }

            void DumpMemberArray(const JsonDDLMemberDef& member)
            {
                if (member.arraySize.value_or(1) > 1)
                {
                    if (member.enum_.has_value())
                        m_stream << "[" << member.enum_.value() << "]";
                    else
                        m_stream << "[" << member.arraySize.value() << "]";
                }
            }

            void DumpMember(const JsonDDLDef& jDDLDef, const JsonDDLStructDef& _struct, const JsonDDLMemberDef& member)
            {
                Indent();
                const auto memberArraySize = member.enum_.has_value() ? jDDLDef.enums[member.link.m_enum_index].members.size() : member.arraySize.value_or(1);
                const auto isStandardSize =
                    T6::DDL::Member::IsStandardSize(static_cast<ddlPrimitiveTypes_e>(member.link.m_type_category), member.link.m_size, memberArraySize);
                //assert(!member.enum_.has_value() || !isStandardSize || member.limits.has_value());
                if (member.link.m_type_category == DDL_STRING_TYPE)
                {
                    m_stream << "string(" << member.maxCharacters.value() << ") " << member.name;
                    DumpMemberArray(member);
                }
                else if (member.link.m_type_category == DDL_STRUCT_TYPE)
                {
                    m_stream << member.type << " " << member.name;
                    DumpMemberArray(member);
                }
                else if (isStandardSize)
                {
                    switch (member.link.m_type_category)
                    {
                    case DDL_BYTE_TYPE:
                        m_stream << "byte ";
                        break;
                    case DDL_SHORT_TYPE:
                        m_stream << "short ";
                        break;
                    case DDL_UINT_TYPE:
                        m_stream << "uint ";
                        break;
                    case DDL_INT_TYPE:
                        m_stream << "int ";
                        break;
                    case DDL_UINT64_TYPE:
                        m_stream << "uint64 ";
                        break;
                    case DDL_FLOAT_TYPE:
                        m_stream << "float ";
                        break;
                    default:
                        assert(false);
                    }

                    m_stream << member.name;
                    DumpMemberArray(member);
                }
                else
                {
                    if (member.link.m_type_category == DDL_FIXEDPOINT_TYPE)
                    {
                        m_stream << "fixed<" << member.limits->fixedMagnitudeBits.value() << "," << member.limits->fixedPrecisionBits.value() << "> ";
                        m_stream << member.name;
                        DumpMemberArray(member);
                    }
                    else if (member.limits->range.has_value())
                    {
                        //m_stream << member.type << "(" << member.limits->range.value() << ",client,server) " << member.name;
                        m_stream << member.type << "(" << member.limits->range.value() << ") " << member.name;
                        DumpMemberArray(member);
                    }
                    else
                    {
                        m_stream << member.type << ":" << member.limits->bits.value() << " " << member.name;
                        DumpMemberArray(member);
                    }
                }

                m_stream << ";\n";
            }

            void DumpStruct(const JsonDDLDef& jDDLDef, const JsonDDLStructDef& _struct, const size_t structIndex)
            {
#ifdef DDL_DEBUG
                Indent();
                m_stream << "// BitOffset: " << _struct.size.value() << "\n";
#endif
                if (!_struct.refCount)
                {
                    Indent();
                    m_stream << "// Unused\n";
                }
                Indent();

                m_stream << "struct " << _struct.name << "\n";

                Indent();
                m_stream << "{\n";

                IncIndent();

                for (const auto& member : _struct.members)
                {
                    DumpMember(jDDLDef, _struct, member);
                }
                
                DecIndent();
                Indent();
                m_stream << "};\n"; // end struct
            }

            void DumpRoot(const JsonDDLDef& jDDLDef, const JsonDDLStructDef& _struct, const size_t structIndex)
            {
                Indent();
                m_stream << "\n";

                std::string permissionScope = "unspecified";
                for (const auto& member : _struct.members)
                {
                    if (permissionScope == "unspecified" && member.permission.value() != "both" && permissionScope != member.permission.value())
                    {
                        permissionScope = member.permission.value();
                        std::string permissionScopeFriendly = permissionScope;
                        permissionScopeFriendly[0] = std::toupper(permissionScope.front());
                        Indent();
                        m_stream << permissionScopeFriendly + ":\n";
                    }

                    DumpMember(jDDLDef, _struct, member);
                }
            }

            void DumpDef(const JsonDDLDef& jDDLDef)
            {
                m_stream << "// ====================\n";
                m_stream << "// Version " << jDDLDef.version << "\n";
                m_stream << "// ====================\n";

#ifdef DDL_DEBUG
                m_stream << "// Size (Bits): " << jDDLDef.size.value() << "\n";
#endif

                m_stream << "version " << jDDLDef.version << "\n{\n";
                IncIndent();

                auto insertEmptyLine = false;

                for (const auto& _enum : jDDLDef.enums)
                {
                    if (insertEmptyLine)
                        m_stream << "\n";
                    else
                        insertEmptyLine = true;

                    DumpEnum(_enum);
                }

                for (auto i = 1; i < jDDLDef.structs.size(); i++)
                {
                    if (insertEmptyLine)
                        m_stream << "\n";
                    else
                        insertEmptyLine = true;

                    DumpStruct(jDDLDef, jDDLDef.structs[i], i);
                }

                DumpRoot(jDDLDef, jDDLDef.structs[0], 0);

                DecIndent();
                m_stream << "}\n"; // end version
            }
        };

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

                        DDLAssert(member.link.m_enum_index == -1 || def.enums.size() == 0 || member.link.m_external_index > 0 || member.link.m_enum_index < def.enums.size() || member.arraySize.value_or(1) == 1);

                        if (member.link.m_enum_index >= 0 && member.arraySize.value_or(1) > 1)
                        {
                            DDLAssert(member.link.m_enum_index < def.enums.size());

                            member.enum_.emplace(def.enums[member.link.m_enum_index].name);

                            DDLAssert(member.arraySize == def.enums[member.link.m_enum_index].members.size());
                        }
                        else
                        {
                            //-1 enumindex indicates no enum
                            //0 enumindex is a reserved value, but is not the same as -1
                            //DDLAssert(member.link.m_enum_index == -1 || def.enums.size() == 0
                            //|| member.link.m_enum_index < def.enums.size() && member.arraySize.value_or(1) != 1);
                            //the above triggering implies a special extra usage of enums
                        }
                    }

                    struc.size.emplace(members.back().link.m_offset + members.back().link.m_size);
                }
            }
        }

        void CreateJsonDDlMemberLimits(JsonDDLMemberLimits& jDDLMemberLimits, const ddlMemberDef_t& ddlMemberDef)
        {
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

                // Int type has the signed bit so the size is calculated differently
                DDLAssert((ddlMemberDef.type != DDL_UINT_TYPE || memberUnitSize == std::bit_width(ddlMemberDef.rangeLimit)));
                DDLAssert((ddlMemberDef.type != DDL_INT_TYPE || memberUnitSize == (std::bit_width(ddlMemberDef.rangeLimit) + 1)));
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
            DDLAssert(ddlMemberDef.type != DDL_INT_TYPE || ddlMemberDef.size >= 1, false);

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
            auto memberUnitSize = (ddlMemberDef.size / ddlMemberDef.arraySize);
            if (ddlMemberDef.type == DDL_STRING_TYPE)
                jDDLMemberDef.maxCharacters.emplace(memberUnitSize / CHAR_BIT);
            else if (ddlMemberDef.type != DDL_STRUCT_TYPE && !T6::DDL::Member::IsStandardSize(typeEnum, ddlMemberDef.size, ddlMemberDef.arraySize))
                CreateJsonDDlMemberLimits(jDDLMemberDef.limits.emplace(jLimits), ddlMemberDef);

            DDLAssert(ddlMemberDef.type != DDL_STRUCT_TYPE || ddlMemberDef.rangeLimit == 0);

            // T7 linker allows type to be enum for a set of flags based on the enum member count, however this appears to be unused in T6
            DDLAssert(ddlMemberDef.type != DDL_ENUM_TYPE);

            jDDLMemberDef.link.m_offset = ddlMemberDef.offset;
            
            jDDLMemberDef.link.m_external_index = ddlMemberDef.externalIndex;

            const auto permissionCategory = static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission);

            DDLAssert(permissionCategory >= DDL_PERMISSIONS_CLIENTONLY && permissionCategory < DDL_PERMISSIONS_COUNT);

            jDDLMemberDef.permission.emplace(T6::DDL::Member::PermissionTypeToName(permissionCategory));

            DDLAssert(jDDLMemberDef.permission.has_value() && jDDLMemberDef.permission.value() != "unknown");
            DDLAssert(ddlMemberDef.arraySize != 1 || (ddlMemberDef.size % ddlMemberDef.arraySize) == 0);
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
            DDLAssert(ddlEnumDef.memberCount > 1 && ddlEnumDef.memberCount < T6::DDL::MAX_MEMBERS);
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
