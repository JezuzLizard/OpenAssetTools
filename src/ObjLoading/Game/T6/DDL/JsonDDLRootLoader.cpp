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

        void ConvertToAsset(const T6::DDL::Def& in, ddlDef_t& out) const
        {
            auto i = 0;
            for (const auto& [k, enum_] : in.m_enums)
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
                i++;
            }

            auto i = 0;
            for (const auto& [k, struc] : in.m_structs)
            {
                auto j = 0;
                for (const auto& [k, inMember] : struc.m_members)
                {
                    auto& outMember = out.structList[i].members[j];

                    outMember.name = m_memory.Dup(inMember.m_name.c_str());
                    outMember.size = inMember.m_link_data.m_size;
                    outMember.offset = inMember.m_link_data.m_offset;
                    outMember.type = inMember.m_link_data.m_type_enum;
                    outMember.externalIndex = inMember.m_link_data.m_external_index;
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
                    outMember.enumIndex = inMember.m_link_data.m_enum_index;
                    outMember.permission = inMember.m_permission.value();
                    j++;
                }

                for (auto j = 0u; j < struc.GetHashTable().size(); j++)
                {
                    out.structList[i].hashTable[j].hash = struc.GetHashTable().at(j).hash;
                    out.structList[i].hashTable[j].index = struc.GetHashTable().at(j).index;
                }
                i++;
            }
        }

        bool ConvertJsonDDLDef(const JsonDDLRoot& jDDLRoot, const JsonDDLDef& jDDLDef, T6::DDL::Def& cDef, bool inInclude) const
        {
            for (const auto& enum_ : jDDLDef.enums)
            {
                T6::DDL::Enum cDDLEnum(cDef);
                cDDLEnum.m_name.AssignLowerCase(enum_.name);
                for (const DDLString member : enum_.members)
                {
                    cDDLEnum.m_members.emplace_back(member);
                }

                if (cDef.m_enums.find(cDDLEnum.m_name) != cDef.m_enums.end())
                {
                    std::cerr << "duplicate enum definition in def\n";
                    return false;
                }

                cDef.m_enums.emplace(cDDLEnum.m_name, cDDLEnum);
            }

            for (const auto& struc : jDDLDef.structs)
            {
                T6::DDL::Struct cDDLStruct(cDef);
                cDDLStruct.m_name.AssignLowerCase(struc.name);
                for (const auto& member : struc.members)
                {
                    T6::DDL::Member cDDLMember(cDDLStruct.m_name, cDDLStruct);
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

                if (cDef.m_structs.find(cDDLStruct.m_name) != cDef.m_structs.end())
                {
                    std::cerr << "duplicate struct definition\n";
                    return false;
                }

                cDef.m_structs.emplace(cDDLStruct.m_name, cDDLStruct);
            }

            if (inInclude)
                return true;

            std::vector<CommonDDLInclude> cDDLIncludes;
            for (auto& [filenames, def] : jDDLRoot.includeDefs)
            {
                T6::DDL::Def cDDLInclude(def.version, filenames.second);
                ConvertJsonDDLDef(jDDLRoot, def, cDDLInclude, true);
            }

            return true;
        }

        bool AllocateDefMembers(const T6::DDL::Def& cDef, ddlDef_t& ddlDef) const
        {
            ddlDef.enumCount = cDef.m_enums.size();
            ddlDef.enumList = m_memory.Alloc<ddlEnumDef_t>(sizeof(ddlEnumDef_t) * ddlDef.enumCount);

            auto i = 0;
            for (const auto& [k, enum_] : cDef.m_enums)
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

            auto i = 0;
            for (const auto& [k, struc] : cDef.m_structs)
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

        bool CreateDDLDef(const T6::DDL::Def& cDef, ddlDef_t& ddlDef) const
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

            for (const auto& cDef : cDDLDefs)
            {
                if (!cDef.Validate())
                    return false;
            }

            for (auto& cDef : cDDLDefs)
            {
                if (!cDef.Calculate())
                    return false;
            }

            auto* ddlDef = m_memory.Alloc<ddlDef_t>();
            auto* firstDef = ddlDef;
            for (const auto& cDef : cDDLDefs)
            {
                if (!CreateDDLDef(cDef, *ddlDef))
                    return false;

                ddlDef->next = m_memory.Alloc<ddlDef_t>();
                ddlDef = ddlDef->next;
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
