#include "JsonDDLRootLoader.h"

#include "Game/T6/DDLConstantsT6.h"
#include "Json/DDL/JsonDDL.h"

#include "Utils/StringUtils.h"

#include <format>
#include <iostream>

using namespace nlohmann;
using namespace T6;

namespace
{
    class JsonLoader
    {
    public:
        JsonLoader(
            std::istream& stream, MemoryManager& memory, IAssetLoadingManager& manager, std::vector<XAssetInfoGeneric*>& dependencies, std::string& rootName)
            : m_stream(stream),
              m_memory(memory),
              m_manager(manager),
              m_dependencies(dependencies),
              m_assetname(rootName)

        {
        }

        #define DDLAssert(expression, ...) DDLAssertInternal(expression, #expression, __VA_ARGS__)

        void DDLAssertInternal(const bool expression,
                               const std::string& expressionString,
                               const bool fatalOnOriginal = true,
                               const bool fatalOnRelink = true) const
        {
            if (!expression)
                std::cout << expressionString << "\n";
            assert(expression);
        }

        bool Load(ddlRoot_t& ddlRoot, ISearchPath& searchPath) const
        {
            const auto jRoot = json::parse(m_stream);
            std::string tool = "";
            std::string type = "";
            size_t version = 0;
            std::string game = "";

            JsonDDLRoot jDDLRoot;

            //if (jRoot.at("_tool"))
            {
                jRoot.at("_tool").get_to(tool);
                utils::MakeStringLowerCase(tool);
            }

            //if (jRoot.at("_type"))
            {
                jRoot.at("_type").get_to(type);
                utils::MakeStringLowerCase(type);
            }

            //if (jRoot.at("_version"))
                jRoot.at("_version").get_to(version);

            //if (jRoot.at("_game"))
            {
                jRoot.at("_game").get_to(game);
                utils::MakeStringLowerCase(game);
            }

            if (type == "" || type != "ddlroot" || version != OAT_DDL_VERSION || game == "" || game != "t6" || tool == "" || tool != "oat")
            {
                std::cerr << "Tried to load ddl \"" << ddlRoot.name << "\" but did not find expected type ddlRoot of version " << OAT_DDL_VERSION << " for game t6\n";
                return false;
            }

            try
            {
                jDDLRoot = jRoot.get<JsonDDLRoot>();
            }
            catch (const json::exception& e)
            {
                std::cerr << std::format("Failed to parse json of ddl: {}\n", e.what());
            }
            return CreateDDLRootFromJson(jDDLRoot, ddlRoot, searchPath);
        }

                bool IsLowerCase(const std::string& string) const
        {
            return std::all_of(string.begin(),
                               string.end(),
                               [](unsigned char c)
                               {
                                   return std::islower(c);
                               });
        }

        bool CheckName(const std::string& name) const
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

        void VerifyLimits(const ddlMemberDef_t& ddlMemberDef) const
        {
            // If this happens it means the assertion that serverDelta, and clientDelta are not assigned separately is false
            DDLAssert((ddlMemberDef.rangeLimit == ddlMemberDef.serverDelta) && (ddlMemberDef.rangeLimit == ddlMemberDef.clientDelta));

            auto memberUnitSize = (ddlMemberDef.size / ddlMemberDef.arraySize);

            if (ddlMemberDef.type == DDL_FIXEDPOINT_TYPE)
            {
                DDLAssert(ddlMemberDef.rangeLimit > 0);
                DDLAssert((memberUnitSize - ddlMemberDef.rangeLimit) > 0);
            }
            else if (ddlMemberDef.rangeLimit != 0)
            {
                DDLAssert(ddlMemberDef.type == DDL_UINT_TYPE || ddlMemberDef.type == DDL_INT_TYPE);

                // Int type has the signed bit so the size is calculated differently
                const auto bitWidth = std::bit_width(ddlMemberDef.rangeLimit);
                DDLAssert((ddlMemberDef.type != DDL_UINT_TYPE || memberUnitSize == bitWidth));
                DDLAssert((ddlMemberDef.type != DDL_INT_TYPE || memberUnitSize == bitWidth + 1));
            }
        }

        void VerifyMember(const ddlMemberDef_t& ddlMemberDef) const
        {
            DDLAssert(CheckName(ddlMemberDef.name));
            DDLAssert(ddlMemberDef.type >= DDL_BYTE_TYPE && ddlMemberDef.type < DDL_TYPE_COUNT);
            DDLAssert(ddlMemberDef.size > 0);
            DDLAssert(ddlMemberDef.arraySize > 0);
            DDLAssert(ddlMemberDef.offset >= 0);
            DDLAssert(ddlMemberDef.externalIndex >= 0 && ddlMemberDef.externalIndex < T6::DDL::MAX_STRUCTS);
            DDLAssert(ddlMemberDef.enumIndex >= -1 && ddlMemberDef.enumIndex < T6::DDL::MAX_ENUMS);
            DDLAssert(ddlMemberDef.type != DDL_STRING_TYPE || (ddlMemberDef.size % CHAR_BIT) == 0);
            DDLAssert(ddlMemberDef.type != DDL_INT_TYPE || ddlMemberDef.size >= 1);

            JsonDDLMemberLimits jLimits;
            auto typeEnum = static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type);

            //.size field has different implications depending on the type
            // string type treat it as max bytes
            // struct is based on the size of the struct
            // enum is based on the type, and also modifies arraySize to the count of its members
            if (ddlMemberDef.type != DDL_STRUCT_TYPE && !T6::DDL::Member::IsStandardSize(typeEnum, ddlMemberDef.size, ddlMemberDef.arraySize))
                VerifyLimits(ddlMemberDef);

            DDLAssert(ddlMemberDef.type != DDL_STRUCT_TYPE || ddlMemberDef.rangeLimit == 0);

            DDLAssert(ddlMemberDef.type != DDL_ENUM_TYPE);

            const auto permissionCategory = static_cast<ddlPermissionTypes_e>(ddlMemberDef.permission);

            DDLAssert(permissionCategory >= DDL_PERMISSIONS_CLIENTONLY && permissionCategory < DDL_PERMISSIONS_COUNT);

            DDLAssert(ddlMemberDef.arraySize != 1 || (ddlMemberDef.size % ddlMemberDef.arraySize) == 0);
        }

        void VerifyStruct(const ddlStructDef_t& ddlStructDef) const
        {
            DDLAssert(CheckName(ddlStructDef.name));
            DDLAssert(ddlStructDef.memberCount > 0 && ddlStructDef.memberCount < T6::DDL::MAX_MEMBERS);
            DDLAssert(ddlStructDef.size > 0);
            DDLAssert(ddlStructDef.members);
            DDLAssert(ddlStructDef.hashTable);

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

#ifdef DDL_FATAL_ERROR_ON_CANON_UNDEFINED_BEHAVIOR
                DDLAssert(prevHash == 0 || ddlStructDef.hashTable[i].hash > prevHash); // This is actually cursed
                DDLAssert(!uniqueIndexes[ddlStructDef.hashTable[i].index]);
                DDLAssert(!uniqueMembers.contains(ddlStructDef.members[i].name)); // HOW?!
#endif

                prevHash = ddlStructDef.hashTable[i].hash;
                uniqueIndexes[ddlStructDef.hashTable[i].index] = true;
                uniqueMembers.insert(ddlStructDef.members[i].name);

                DDLAssert(ddlStructDef.members[i].offset <= ddlStructDef.size);
                DDLAssert(ddlStructDef.members[i].offset == 0 || ddlStructDef.members[i].offset > prevOffset);
                DDLAssert(ddlStructDef.members[i].offset == calculatedStructSize);
                DDLAssert(ddlStructDef.members[i].size <= ddlStructDef.size);

                prevOffset = ddlStructDef.members[i].offset;
                calculatedStructSize += ddlStructDef.members[i].size;

                VerifyMember(ddlStructDef.members[i]);
            }

            DDLAssert(ddlStructDef.size == calculatedStructSize);
        }

        void VerifyEnum(const ddlEnumDef_t& ddlEnumDef) const
        {
            DDLAssert(CheckName(ddlEnumDef.name));
            DDLAssert(ddlEnumDef.memberCount > 1 && ddlEnumDef.memberCount < T6::DDL::MAX_MEMBERS);
            DDLAssert(ddlEnumDef.members && ddlEnumDef.members[0]);
            DDLAssert(ddlEnumDef.hashTable);

            auto prevHash = 0;
            std::vector<bool> uniqueIndexes;
            uniqueIndexes.resize(ddlEnumDef.memberCount);

            std::set<std::string> uniqueMembers;

            for (auto i = 0; i < ddlEnumDef.memberCount; i++)
            {
                DDLAssert(ddlEnumDef.hashTable[i].hash);
                DDLAssert(ddlEnumDef.hashTable[i].index >= 0 && ddlEnumDef.hashTable[i].index < ddlEnumDef.memberCount);

#ifdef DDL_FATAL_ERROR_ON_CANON_UNDEFINED_BEHAVIOR
                DDLAssert(prevHash == 0 || ddlEnumDef.hashTable[i].hash > prevHash);
                DDLAssert(!uniqueIndexes[ddlEnumDef.hashTable[i].index]);
                DDLAssert(!uniqueMembers.contains(ddlEnumDef.members[i]));
#endif

                prevHash = ddlEnumDef.hashTable[i].hash;
                uniqueIndexes[ddlEnumDef.hashTable[i].index] = true;
                uniqueMembers.insert(ddlEnumDef.members[i]);

                DDLAssert(CheckName(ddlEnumDef.members[i]));
            }
        }

        void VerifyDef(int* prevDefVersion, const ddlDef_t* ddlDef) const
        {
            DDLAssert(ddlDef->version > 0);
            DDLAssert(*prevDefVersion == 0 || *prevDefVersion > ddlDef->version);
            DDLAssert(ddlDef->size > 0);
            DDLAssert(ddlDef->structCount > 0 && ddlDef->structCount < T6::DDL::MAX_STRUCTS);
            DDLAssert(ddlDef->enumCount >= 0 && ddlDef->enumCount < T6::DDL::MAX_ENUMS);

            *prevDefVersion = ddlDef->version;

            DDLAssert(ddlDef->structList != nullptr);
            DDLAssert(!strncmp(ddlDef->structList[0].name, "root", sizeof("root")));

            DDLAssert(ddlDef->enumCount != 0 || ddlDef->enumList == nullptr);
        }

        void ConvertToAsset(const CommonDDLDef& in, ddlDef_t& out) const
        {
            auto calculatedSize = 0;
            auto i = 0;
            for (const auto& enum_ : in.m_enums)
            {
                for (auto j = 0u; j < enum_.m_members.size(); j++)
                {
                    out.enumList[i].members[j] = m_memory.Dup(enum_.m_members[j].c_str());
                }
                for (auto j = 0u; j < enum_.GetHashTable().size(); j++)
                {
                    auto& hashTable = enum_.GetHashTable().at(j);
                    out.enumList[i].hashTable[j].hash = hashTable.hash;
                    out.enumList[i].hashTable[j].index = hashTable.index;
                }
                VerifyEnum(out.enumList[i]);
                i++;
            }

            i = 0;
            for (const auto& struc : in.m_structs)
            {
                auto j = 0;
                for (const auto& inMember : struc.m_members)
                {
                    auto& outMember = out.structList[i].members[j];

                    outMember.name = m_memory.Dup(inMember.m_name.c_str());
                    outMember.size = inMember.m_size;
                    outMember.offset = inMember.m_offset;
                    outMember.type = inMember.m_type_category;
                    outMember.externalIndex = inMember.m_external_index;
                    if (inMember.m_limits.has_value())
                    {
                        if (inMember.m_limits->m_range.has_value())
                        {
                            outMember.rangeLimit = inMember.m_limits->m_range.value();
                            outMember.serverDelta = inMember.m_limits->m_range.value();
                            outMember.clientDelta = inMember.m_limits->m_range.value();
                        }
                    }
                    outMember.arraySize = inMember.m_array_size.value_or(1);
                    outMember.enumIndex = inMember.m_enum_index;
                    outMember.permission = inMember.m_permission.value();
                    
                    DDLAssert(out.structList[i].size == outMember.offset);
                    out.structList[i].size += outMember.size;       
                    j++;
                }

                out.structList[i].size = struc.m_size;

                for (auto j = 0u; j < struc.GetHashTable().size(); j++)
                {
                    out.structList[i].hashTable[j].hash = struc.GetHashTable().at(j).hash;
                    out.structList[i].hashTable[j].index = struc.GetHashTable().at(j).index;
                }
                VerifyStruct(out.structList[i]);
                i++;
            }

            DDLAssert(out.size == out.structList[0].size);
        }

        bool ConvertJsonDDLDef(const JsonDDLRoot& jDDLRoot, const JsonDDLDef& jDDLDef, CommonDDLRoot& cRoot, CommonDDLDef& cDef, bool inInclude) const
        {
            auto i = 0u;
            for (const auto& enum_ : jDDLDef.enums)
            {
                std::string lowerCopy;
                lowerCopy = enum_.name;
                utils::MakeStringLowerCase(lowerCopy);
                CommonDDLEnumDef cDDLEnum(lowerCopy, &cDef, i);
                for (const std::string member : enum_.members)
                {
                    cDDLEnum.m_members.emplace_back(member);
                }

                auto it = std::find_if(cDef.m_enums.begin(),
                                       cDef.m_enums.end(),
                                       [cDDLEnum](const CommonDDLEnumDef& _enum)
                                       {
                                           return cDDLEnum.m_name == _enum.m_name;
                                       });

                if (it != cDef.m_enums.end())
                {
                    std::cerr << "duplicate enum definition in def\n";
                    return false;
                }

                cDef.m_enums.push_back(cDDLEnum);
                i++;
            }

            i = 0u;
            for (const auto& struc : jDDLDef.structs)
            {
                std::string lowerCopy;
                lowerCopy = struc.name;
                utils::MakeStringLowerCase(lowerCopy);
                CommonDDLStructDef cDDLStruct(lowerCopy, &cDef, i);

                for (const auto& member : struc.members)
                {
                    std::string lowerCopyName = member.name;
                    utils::MakeStringLowerCase(lowerCopyName);
                    CommonDDLMemberDef cDDLMember(lowerCopyName, &cDDLStruct);
                    if (member.enum_.has_value())
                    {
                        lowerCopy = member.enum_.value();
                        utils::MakeStringLowerCase(lowerCopy);
                        cDDLMember.m_enum.emplace(lowerCopy);
                    }

                    std::string lowerCopyType = member.type;
                    utils::MakeStringLowerCase(lowerCopyType);
                    cDDLMember.m_type = lowerCopyType;
                    if (struc.name == "root" && member.permission.has_value())
                        cDDLMember.m_permission.emplace(cDDLMember.NameToPermissionType(member.permission.value()));
                    cDDLMember.m_array_size = member.arraySize;
                    if (member.limits.has_value())
                        cDDLMember.m_limits.emplace(member.limits->bits, member.limits->range, member.limits->fixedPrecisionBits, member.limits->fixedMagnitudeBits);
                    
                    cDDLMember.m_max_characters = member.maxCharacters;

                    /*
                    auto it = std::find_if(cDDLStruct.m_members.begin(),
                                           cDDLStruct.m_members.end(),
                            [cDDLMember](const CommonDDLMemberDef& member)
                            {
                                 return cDDLMember.m_name == member.m_name;
                            });

                    if (it != cDDLStruct.m_members.end())
                    {
                        std::cerr << "duplicate member definition in struct\n";
                        return false;
                    }
                    */

                    cDDLStruct.m_members.push_back(cDDLMember);
                }

                auto it = std::find_if(cDef.m_structs.begin(),
                                       cDef.m_structs.end(),
                                       [cDDLStruct](const CommonDDLStructDef& struc)
                    {
                                           return cDDLStruct.m_name == struc.m_name;
                    });

                if (it != cDef.m_structs.end())
                {
                    std::cerr << "duplicate struct definition\n";
                    return false;
                }

                cDef.m_structs.push_back(cDDLStruct);
                i++;
            }

            if (inInclude)
                return true;

            // this should be detected before this
            /*
            assert(jDDLRoot.includeDefs.contains(jDDLDef.filename));

            for (auto& includeDef : jDDLRoot.includeDefs.at(jDDLDef.filename))
            {
                CommonDDLDef cDDLDefInclude(includeDef.version, includeDef.filename, cRoot, true);
                ConvertJsonDDLDef(jDDLRoot, includeDef, cRoot, cDDLDefInclude, true);
            }
            */

            return true;
        }

        bool AllocateDefMembers(const CommonDDLDef& cDef, ddlDef_t& ddlDef) const
        {
            ddlDef.enumCount = cDef.m_enums.size();
            if (ddlDef.enumCount > 0)
            {
                ddlDef.enumList = m_memory.Alloc<ddlEnumDef_t>(sizeof(ddlEnumDef_t) * ddlDef.enumCount);
            }
            else
            {
                ddlDef.enumList = nullptr;
            }

            auto i = 0;
            for (const auto& enum_ : cDef.m_enums)
            {
                ddlDef.enumList[i].name = m_memory.Dup(enum_.m_name.c_str());
                ddlDef.enumList[i].memberCount = enum_.m_members.size();
                ddlDef.enumList[i].members = m_memory.Alloc<const char*>(sizeof(const char*) * ddlDef.enumList[i].memberCount);
                ddlDef.enumList[i].hashTable = m_memory.Alloc<ddlHash_t>(sizeof(ddlHash_t) * ddlDef.enumList[i].memberCount);
                i++;
            }

            assert(i == cDef.m_enums.size());

            ddlDef.structCount = cDef.m_structs.size();
            ddlDef.structList = m_memory.Alloc<ddlStructDef_t>(sizeof(ddlStructDef_t) * ddlDef.structCount);

            i = 0;
            for (const auto& struc : cDef.m_structs)
            {
                ddlDef.structList[i].name = m_memory.Dup(struc.m_name.c_str());
                ddlDef.structList[i].memberCount = struc.m_members.size();
                ddlDef.structList[i].members = m_memory.Alloc<ddlMemberDef_t>(sizeof(ddlMemberDef_t) * ddlDef.structList[i].memberCount);
                ddlDef.structList[i].hashTable = m_memory.Alloc<ddlHash_t>(sizeof(ddlHash_t) * ddlDef.structList[i].memberCount);
                i++;
            }

            assert(i == cDef.m_structs.size());

            return true;
        }

        bool CreateDDLDef(const CommonDDLDef& cDef, ddlDef_t& ddlDef) const
        {
            ddlDef.version = cDef.m_version;
            ddlDef.size = cDef.m_size;
            if (!AllocateDefMembers(cDef, ddlDef))
                return false;
            
            ConvertToAsset(cDef, ddlDef);

            return true;
        }

        bool LoadDDLDefJson(const std::string defFilename, ISearchPath& searchPath, JsonDDLDef& jDDLDef) const
        {
            jDDLDef.filename = defFilename;
            const auto secondaryAssetFile = searchPath.Open(defFilename);
            if (!secondaryAssetFile.IsOpen())
            {
                std::cerr << "couldn't open ddlroot dependency \"" << defFilename << "\" for \"" << m_assetname << "\n ";
                return false;
            }

            const auto jRoot = json::parse(*secondaryAssetFile.m_stream);
            std::string tool;
            std::string type;
            unsigned int version;
            std::string game;

            jRoot.at("_tool").get_to(tool);
            jRoot.at("_type").get_to(type);
            jRoot.at("_version").get_to(version);
            jRoot.at("_game").get_to(game);

            utils::MakeStringLowerCase(tool);
            utils::MakeStringLowerCase(type);
            utils::MakeStringLowerCase(game);

            if (tool != "oat")
            {
                std::cerr << "Tried to load ddldef for \"" << m_assetname << "\" but asset format isn't for oat\n";
                return false;
            }

            if (type != "ddldef" || version != OAT_DDL_VERSION || game != "t6")
            {
                std::cerr << "Tried to load ddldef for \"" << m_assetname << "\" but did not find expected type ddldef of version " << OAT_DDL_VERSION
                          << " for game t6\n";
                return false;
            }

            try
            {
                jDDLDef = jRoot["def"].get<JsonDDLDef>();
                return true;
            }
            catch (const json::exception& e)
            {
                std::cerr << std::format("Failed to parse json of ddldef: {}\n", e.what());
            }

            return false;
        }

        bool CreateDDLRootFromJson(JsonDDLRoot& jDDLRoot, ddlRoot_t& ddlRoot, ISearchPath& searchPath) const
        {
            std::string rootName(ddlRoot.name);
            CommonDDLRoot cDDLRoot(rootName);
            std::vector<CommonDDLDef> cDDLT6Defs;
            jDDLRoot.filename = rootName;

            if (!jDDLRoot.defFiles.size())
            {
                std::cerr << "ddl \"" << m_assetname << "\" has no def file entries\n";
                return false;
            }

            // commonddldef structure supports multiple defs in the same file, but jsonddl does not
            std::vector<JsonDDLDef> jDDLDefs;
            jDDLDefs.resize(jDDLRoot.defFiles.size());
            for (auto i = 0u; i < jDDLRoot.defFiles.size(); i++)
            {
                if (!LoadDDLDefJson(jDDLRoot.defFiles[i], searchPath, jDDLDefs[i]))
                    return false;

                if (jDDLDefs[i].enums.size() > T6::DDL::MAX_ENUMS)
                    return false;

                for (const auto& enum_ : jDDLDefs[i].enums)
                    if (enum_.members.size() > T6::DDL::MAX_MEMBERS)
                        return false;

                if (jDDLDefs[i].structs.size() > T6::DDL::MAX_STRUCTS)
                    return false;

                for (const auto& struc : jDDLDefs[i].structs)
                    if (struc.members.size() > T6::DDL::MAX_MEMBERS)
                        return false;

                /*
                for (auto& jDDLIncludeFile : jDDLRoot.defs[i].includeFiles)
                {
                    JsonDDLDef jDDLInclude;
                    if (!LoadDDLDefJson(jDDLIncludeFile, searchPath, jDDLInclude))
                        return false;

                    //jDDLRoot.includeDefs.insert_or_assign(jDDLIncludeFile, jDDLInclude);
                }
                */
            }

            std::sort(jDDLDefs.begin(),
                      jDDLDefs.end(),
                      [](const JsonDDLDef& a, const JsonDDLDef& b)
                      {
                          return a.version >= b.version;
                      });

            for (auto i = 0u; i < jDDLDefs.size(); i++)
            {
                CommonDDLDef cDDLDef(jDDLDefs[i].version, jDDLRoot.defFiles[i], cDDLRoot, false);
                if (!ConvertJsonDDLDef(jDDLRoot, jDDLDefs[i], cDDLRoot, cDDLDef, false))
                    return false;

                cDDLRoot.m_defs[jDDLRoot.defFiles[i]].push_back(cDDLDef);
                cDDLT6Defs.push_back(cDDLDef);
            }

            for (auto& cDef : cDDLT6Defs)
            {
                for (auto& cEnum : cDef.m_enums)
                {
                    cEnum.SetParentDef(&cDef);
                }
                for (auto& cStruct : cDef.m_structs)
                {
                    cStruct.SetParentDef(&cDef);
                    for (auto& cMember : cStruct.m_members)
                    {
                        cMember.SetParentStruct(&cStruct);
                    }
                }
            }

            for (auto& cDef : cDDLT6Defs)
            {
                if (!cDef.Resolve())
                    return false;
            }

            for (auto& cDef : cDDLT6Defs)
            {
                cDef.m_in_calculation.resize(cDef.m_structs.size(), false);
                if (!cDef.Validate())
                    return false;
            }

            for (auto& cDef : cDDLT6Defs)
            {
                if (!cDef.Calculate())
                    return false;
            }

            auto* ddlDef = m_memory.Alloc<ddlDef_t>();
            auto* firstDef = ddlDef;
            auto i = 0;
            int prevDefVersion = 0;
            for (auto& cDef : cDDLT6Defs)
            {
                if (!CreateDDLDef(cDef, *ddlDef))
                    return false;

                VerifyDef(&prevDefVersion, ddlDef);

                ddlDef->next = i < (cDDLT6Defs.size() - 1) ? m_memory.Alloc<ddlDef_t>() : 0;
                if (!ddlDef->next)
                {
                    break;
                }
                ddlDef = ddlDef->next;
                i++;
            }

            ddlRoot.ddlDef = std::move(firstDef);

            return true;
        }
        std::istream& m_stream;
        MemoryManager& m_memory;
        IAssetLoadingManager& m_manager;
        std::vector<XAssetInfoGeneric*>& m_dependencies;
        std::string& m_assetname;
    };
} // namespace

namespace T6
{
    bool LoadDDLRootAsJson(std::istream& stream,
                           ddlRoot_t& ddlRoot,
                           ISearchPath* searchPath,
                           MemoryManager* memory,
                           IAssetLoadingManager* manager,
                           std::vector<XAssetInfoGeneric*>& dependencies)
    {
        std::string assetName(ddlRoot.name);
        const JsonLoader loader(stream, *memory, *manager, dependencies, assetName);

        return loader.Load(ddlRoot, *searchPath);
    }
} // namespace T6
