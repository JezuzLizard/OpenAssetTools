#include "JsonDDLRootLoader.h"

#include "Game/T6/DDLConstantsT6.h"
#include "Json/DDL/JsonDDL.h"

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

        bool Load(ddlRoot_t& ddlRoot, ISearchPath& searchPath) const
        {
            const auto jRoot = json::parse(m_stream);
            std::optional<std::string> tool;
            std::optional<std::string> type;
            std::optional<size_t> version;
            std::optional<std::string> game;

            JsonDDLRoot jDDLRoot;

            /*
            if (jRoot.at("_tool"))
            {
                jRoot.at("_tool").get_to(tool);
                utils::MakeStringLowerCase(*tool);
            }

            if (jRoot.at("_type"))
            {
                jRoot.at("_type").get_to(type.value());
                utils::MakeStringLowerCase(*type);
            }

            if (jRoot.at("_version"))
                jRoot.at("_version").get_to(version.value());

            if (jRoot.at("_game"))
            {
                jRoot.at("_game").get_to(game.value());
                utils::MakeStringLowerCase(*game);
            }

            if (!type.has_value() || type.value() != "ddlroot" || version.value_or(0) != OAT_DDL_VERSION || !game.has_value() || game.value() != "t6"
                || !tool.has_value() || tool.value() != "oat")
            {
                std::cerr << "Tried to load ddl \"" << ddlRoot.name << "\" but did not find expected type ddlRoot of version " << OAT_DDL_VERSION << " for game t6\n";
                return false;
            }
            */

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

        /*

        inline void from_json(const JsonDDLDef& in, ddlDef_t& out) const
        {
            for (auto i = 0; i < out.enumCount; i++)
            {
                for (auto j = 0; j < out.enumList[i].memberCount; j++)
                {
                    out.enumList[i].members[j] = m_memory.Dup(in.enums[i].members[j].c_str());
                }
                for (auto j = 0; j < out.enumList[i].memberCount; j++)
                {
                    out.enumList[i].hashTable[j] = in.enums[i].sortedHashTable.at(j);
                }
            }

            for (auto i = 0; i < out.structCount; i++)
            {
                for (auto j = 0; j < out.structList[i].memberCount; j++)
                {
                    const auto& inMember = in.structs[i].members[j];
                    auto& outMember = out.structList[i].members[j];
                    
                    outMember.name = m_memory.Dup(inMember.name.c_str());
                    outMember.size = inMember.data.m_link_data.size;
                    outMember.offset = inMember.data.m_link_data.offset;
                    outMember.type = inMember.data.m_link_data.typeEnum;
                    outMember.externalIndex = inMember.data.m_link_data.externalIndex;
                    if (inMember.limits)
                    {
                        if (inMember.limits->range)
                        {
                            outMember.rangeLimit = inMember.limits->range.value();
                            outMember.serverDelta = inMember.limits->range.value();
                            outMember.clientDelta = inMember.limits->range.value();
                        }
                    }
                    outMember.arraySize = inMember.arraySize.value_or(1);
                    outMember.enumIndex = inMember.data.m_link_data.enumIndex;
                    outMember.permission = inMember.permission.value();
                }
                for (auto j = 0; j < out.structList[i].memberCount; j++)
                {
                    out.structList[i].hashTable[j] = in.structs[i].sortedHashTable.at(j);
                }
            }
        }
        */

        bool ConvertJsonDDLDef(const JsonDDLRoot& jDDLRoot, const JsonDDLDef& jDDLDef, T6::DDL::Def& cDDLDef, bool inInclude) const
        {
            for (const auto& enum_ : jDDLDef.enums)
            {
                T6::DDL::Enum cDDLEnum(cDDLDef);
                cDDLEnum.m_name.AssignLowerCase(enum_.name);
                for (const DDLString member : enum_.members)
                {
                    cDDLEnum.m_members.push_back(member);
                }

                if (cDDLDef.m_enums.find(cDDLEnum.m_name) != cDDLDef.m_enums.end())
                {
                    std::cerr << "duplicate enum definition in def\n";
                    return false;
                }

                cDDLDef.m_enums.emplace(cDDLEnum.m_name, cDDLEnum);
            }

            for (const auto& struc : jDDLDef.structs)
            {
                T6::DDL::Struct cDDLStruct(cDDLDef);
                cDDLStruct.m_name.AssignLowerCase(struc.name);
                for (const auto& member : struc.members)
                {
                    T6::DDL::Member cDDLMember(cDDLStruct);
                    if (member.enum_.has_value())
                        cDDLMember.m_enum->AssignLowerCase(member.enum_.value());
                    cDDLMember.m_name.AssignLowerCase(member.name);
                    cDDLMember.m_type.AssignLowerCase(member.type);
                    if (member.permission.has_value())
                        cDDLMember.m_permission = cDDLMember.NameToPermissionType(member.permission.value());
                    cDDLMember.m_array_size = member.arraySize;
                    if (member.limits.has_value())
                        cDDLMember.m_limits.emplace(member.limits->bits, member.limits->range, member.limits->fixedPrecisionBits, member.limits->fixedMagnitudeBits);
                    
                    cDDLMember.m_max_characters = member.maxCharacters;

                    if (cDDLStruct.m_members.find(cDDLMember.m_name) != cDDLStruct.m_members.end())
                    {
                        std::cerr << "duplicate member definition in struct\n";
                        return false;
                    }

                    cDDLStruct.m_members.emplace(cDDLMember.m_name, cDDLMember);
                }

                if (cDDLDef.m_structs.find(cDDLStruct.m_name) != cDDLDef.m_structs.end())
                {
                    std::cerr << "duplicate struct definition\n";
                    return false;
                }

                cDDLDef.m_structs.emplace(cDDLStruct.m_name, cDDLStruct);
            }

            std::vector<CommonDDLInclude> cDDLIncludes;
            for (auto& [filenames, def] : jDDLRoot.includeDefs)
            {
                T6::DDL::Def cDDLInclude(def.version, filenames.second);
                ConvertJsonDDLDef(jDDLRoot, def, cDDLInclude, true);
            }
        }

        bool AllocateDefMembers(const JsonDDLDef& jDDLDef, ddlDef_t& ddlDef) const
        {
            ddlDef.enumCount = jDDLDef.enums.size();
            ddlDef.enumList = m_memory.Alloc<ddlEnumDef_t>(sizeof(ddlEnumDef_t) * ddlDef.enumCount);
            for (auto i = 0; i < ddlDef.enumCount; i++)
            {
                ddlDef.enumList[i].name = m_memory.Dup(jDDLDef.enums[i].name.c_str());
                ddlDef.enumList[i].memberCount = jDDLDef.enums[i].members.size();
                ddlDef.enumList[i].members = m_memory.Alloc<const char*>(sizeof(char*) * ddlDef.enumList[i].memberCount);
                ddlDef.enumList[i].hashTable = m_memory.Alloc<ddlHash_t>(sizeof(ddlHash_t) * ddlDef.enumList[i].memberCount);
            }

            ddlDef.structCount = jDDLDef.structs.size();
            ddlDef.structList = m_memory.Alloc<ddlStructDef_t>(sizeof(ddlStructDef_t) * ddlDef.structCount);
            for (auto i = 0; i < ddlDef.structCount; i++)
            {
                ddlDef.structList[i].name = m_memory.Dup(jDDLDef.structs[i].name.c_str());
                ddlDef.structList[i].memberCount = jDDLDef.structs[i].members.size();
                ddlDef.structList[i].members = m_memory.Alloc<ddlMemberDef_t>(sizeof(ddlMemberDef_t) * ddlDef.structList[i].memberCount);
                ddlDef.structList[i].hashTable = m_memory.Alloc<ddlHash_t>(sizeof(ddlHash_t) * ddlDef.structList[i].memberCount);
            }

            return true;
        }

        bool CreateDDLDefFromJson(JsonDDLDef& jDDLDef, ddlDef_t& ddlDef) const
        {
            if (!jDDLDef.Calculate())
                return false;

            ddlDef.version = jDDLDef.version;
            ddlDef.size = jDDLDef.size.value();
            if (!AllocateDefMembers(jDDLDef, ddlDef))
                return false;
            
            from_json(jDDLDef, ddlDef);

            return true;
        }

        bool LoadDDLDefJson(const std::string defFilename, ISearchPath& searchPath, JsonDDLDef& jDDLDef) const
        {
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

            /*
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
            */

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
            // Canonical limits in T7 linker
            constexpr size_t MAX_STRUCTS = 48;
            constexpr size_t MAX_ENUMS = 32;
            constexpr size_t MAX_MEMBERS = 1023;

            std::vector<T6::DDL::Def> cDDLDefs;
            if (!jDDLRoot.defFiles.size())
            {
                std::cerr << "ddl \"" << m_assetname << "\" has no def file entries\n";
                return false;
            }

            for (auto i = 0u; i < jDDLRoot.defFiles.size(); i++)
            {
                JsonDDLDef jDDLDef;
                if (!LoadDDLDefJson(jDDLRoot.defFiles[i], searchPath, jDDLDef))
                    return false;

                if (jDDLDef.enums.size() > MAX_ENUMS)
                    return false;

                for (const auto& enum_ : jDDLDef.enums)
                    if (enum_.members.size() > MAX_MEMBERS)
                        return false;

                if (jDDLDef.structs.size() > MAX_STRUCTS)
                    return false;

                for (const auto& struc : jDDLDef.structs)
                    if (struc.members.size() > MAX_MEMBERS)
                        return false;

                for (auto& jDDLIncludeFile : jDDLRoot.defs[i].includeFiles)
                {
                    JsonDDLDef jDDLInclude;
                    if (!LoadDDLDefJson(jDDLIncludeFile, searchPath, jDDLInclude))
                        return false;
                    jDDLRoot.includeDefs.insert_or_assign({jDDLRoot.defFiles[i], jDDLIncludeFile}, jDDLInclude);
                }
                T6::DDL::Def cDDLDef(jDDLDef.version, jDDLRoot.defFiles[i]);
                if (!ConvertJsonDDLDef(jDDLRoot, jDDLDef, cDDLDef, false))
                    return false;

                cDDLDefs.push_back(cDDLDef);
            }

            for (auto& jDef : jDDLDefs)
            {
                if (!jDef.Validate())
                    return false;
            }

            auto* ddlDef = m_memory.Alloc<ddlDef_t>();
            auto* firstDef = ddlDef;
            for (auto i = 0u; i < jDDLDefs.size(); i++)
            {
                if (i > 0u)
                {
                    ddlDef->next = m_memory.Alloc<ddlDef_t>();
                    ddlDef = ddlDef->next;
                }
                if (!CreateDDLDefFromJson(jDDLDefs[i], *ddlDef))
                    return false;
            }

            ddlRoot.ddlDef = firstDef;

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
