#include "JsonDDLRootLoader.h"

#include "Game/T6/CommonT6.h"
#include "Game/T6/Json/DDL/JsonDDLRoot.h"

#include "Utils/StringUtils.h"

#include <format>
#include <iostream>
#include <nlohmann/json.hpp>

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
            std::string type;
            unsigned version;
            std::string game;

            jRoot.at("_type").get_to(type);
            jRoot.at("_version").get_to(version);
            jRoot.at("_game").get_to(game);

            utils::MakeStringLowerCase(type);
            utils::MakeStringLowerCase(game);

            if (type != "ddlroot" || version != OAT_DDL_VERSION || game != "t6")
            {
                std::cerr << "Tried to load ddl \"" << ddlRoot.name << "\" but did not find expected type ddlRoot of version " << OAT_DDL_VERSION << " for game t6\n";
                return false;
            }

            try
            {
                auto jDDLRoot = jRoot.get<JsonDDLRoot>();
                return CreateDDLRootFromJson(jDDLRoot, ddlRoot, searchPath);
            }
            catch (const json::exception& e)
            {
                std::cerr << std::format("Failed to parse json of ddl: {}\n", e.what());
            }
            return false;
        }

    private:

        inline void from_json(const JsonDDLDef& in, ddlDef_t& out)
        {
            for (auto i = 0u; i < out.enumCount; i++)
            {
                for (auto j = 0u; j < out.enumList[i].memberCount; j++)
                {
                    out.enumList[i].members[j] = in.enums[i].members[j].c_str();
                }
                for (auto j = 0u; j < out.enumList[i].memberCount; j++)
                {
                    out.enumList[i].hashTable[j].hash = Common::Com_HashString(out.enumList[i].members[j]);
                    out.enumList[i].hashTable[j].index = in.enums[i].hashTable.at(out.enumList[i].hashTable[j].hash);
                }
            }

            for (auto i = 0; i < out.structCount; i++)
            {
                for (auto j = 0; j < out.structList[i].memberCount; j++)
                {
                    out.structList[i].members[j].arraySize = in.structs[i].members[j].arrayCount.value_or(1);
                    if (in.structs[i].members[j].limits.has_value())
                    {
                        if (in.structs[i].members[j].limits.value().bits.has_value())
                        {
                            out.structList[i].members[j].
                        }
                    }
                    out.structList[i].members[j].size = in.structs[i].members[j].memberSize.value();
                    out.structList[i].members[j].clientDelta
                }
                for (auto j = 0u; j < out.structList[i].memberCount; j++)
                {
                    out.structList[i].hashTable[j].hash = Common::Com_HashString(out.structList[i].members[j].name);
                    out.structList[i].hashTable[j].index = in.structs[i].hashTable.at(out.structList[i].hashTable[j].hash);
                }
            }
        };

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
            jDDLDef.Calculate();

            ddlDef.version = jDDLDef.version;
            if (!AllocateDefMembers(jDDLDef, ddlDef))
                return false;

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
            std::string type;
            unsigned version;
            std::string game;

            jRoot.at("_type").get_to(type);
            jRoot.at("_version").get_to(version);
            jRoot.at("_game").get_to(game);

            utils::MakeStringLowerCase(type);
            utils::MakeStringLowerCase(game);

            if (type != "ddldef" || version != OAT_DDL_VERSION || game != "t6")
            {
                std::cerr << "Tried to load ddldef for \"" << m_assetname << "\" but did not find expected type ddlDef of version " << OAT_DDL_VERSION
                          << " for game t6\n";
                return false;
            }

            try
            {
                jDDLDef = jRoot.get<JsonDDLDef>();
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
            std::vector<JsonDDLDef> jDDLDefs;
            if (!jDDLRoot.defFiles.size())
            {
                std::cerr << "ddl \"" << m_assetname << "\" has no def file entries\n";
                return false;
            }

            for (auto i = 0u; i < jDDLRoot.defFiles.size(); i++)
            {
                JsonDDLDef jDDLDef;
                jDDLDef.filename.assign(jDDLRoot.defFiles[i]);
                if (!LoadDDLDefJson(jDDLRoot.defFiles[i], searchPath, jDDLDef))
                    return false;

                jDDLDefs.push_back(jDDLDef);
            }

            for (auto i = 0u; i < jDDLDefs.size(); i++)
            {
                jDDLDefs[i].Validate();
            }

            auto* ddlDef = m_memory.Alloc<ddlDef_t>();
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
