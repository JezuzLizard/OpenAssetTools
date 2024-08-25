#include "JsonDDLRootLoader.h"

#include "Game/T6/CommonT6.h"
#include "Game/T6/Json/JsonDDLRoot.h"

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
                const auto jDDLRoot = jRoot.get<JsonDDLRoot>();
                return CreateDDLRootFromJson(jDDLRoot, ddlRoot, searchPath);
            }
            catch (const json::exception& e)
            {
                std::cerr << std::format("Failed to parse json of ddl: {}\n", e.what());
            }
            return false;
        }

    private:
        class Validate
        {
        private:
            static bool Root(const JsonDDLDef& jDDLDef)
            {
                std::string lowerName;
                for (auto i = 0u; i < jDDLDef.structs.size(); i++)
                {
                    lowerName.assign(jDDLDef.structs[i].name);
                    utils::MakeStringLowerCase(lowerName);

                    if (lowerName == "root")
                        return true;
                }

                return false;
            }

            class Limits
            {
                static bool FixedPoint(const JsonDDLMemberDef& jDDLMemberDef)
                {
                    if (!jDDLMemberDef.limits.has_value())
                        return true;

                    const auto& limits = jDDLMemberDef.limits.value();
                    if (!limits.fixedMagnitudeBits.has_value() || !limits.fixedPrecisionBits.has_value())
                        return true;

                    const auto typeEnum = NameToType(jDDLMemberDef.type);
                    if (typeEnum != DDL_FIXEDPOINT_TYPE)
                        return false;
                    if (limits.range.has_value() || limits.bits.has_value())
                        return false;
                    if ((limits.fixedMagnitudeBits.value() + limits.fixedPrecisionBits.value()) > DDL::typeFeatures[typeEnum].size)
                        return false;

                    return true;
                }

                static bool Range(const JsonDDLMemberDef& jDDLMemberDef)
                {
                    if (!jDDLMemberDef.limits.has_value())
                        return true;

                    const auto& limits = jDDLMemberDef.limits.value();
                    if (!limits.range.has_value())
                        return true;

                    const auto typeEnum = NameToType(jDDLMemberDef.type);
                    if ((DDL::typeFeatures[typeEnum].flags & DDL_FLAG_LIMITS) == 0)
                        return false;
                    if (limits.bits.has_value() || limits.fixedPrecisionBits.has_value() || limits.fixedMagnitudeBits.has_value())
                        return false;
                    if (limits.range.value() > DDL::typeFeatures[typeEnum].max)
                        return false;

                    return true;
                }

                static bool BitFields(const JsonDDLMemberDef& jDDLMemberDef)
                {
                    if (!jDDLMemberDef.limits.has_value())
                        return true;

                    const auto& limits = jDDLMemberDef.limits.value();
                    if (!limits.bits.has_value())
                        return true;

                    const auto typeEnum = NameToType(jDDLMemberDef.type);
                    if ((DDL::typeFeatures[typeEnum].flags & DDL_FLAG_BITFIELDS) == 0)
                        return false;
                    if (limits.range.has_value() || limits.fixedPrecisionBits.has_value() || limits.fixedMagnitudeBits.has_value())
                        return false;
                    if (limits.bits.value() > DDL::typeFeatures[typeEnum].size)
                        return false;

                    return true;
                }
            };

            class Struct
            {
            private:
                static bool Name(const JsonDDLDef& jDDLDef, const JsonDDLStructDef& jDDLStructDef)
                {
                    std::string lowerName1(jDDLStructDef.name);
                    utils::MakeStringLowerCase(lowerName1);
                    std::string lowerName2;

                    size_t nameRedefinitions = 0;
                    for (auto i = 0u; i < jDDLDef.structs.size(); i++)
                    {
                        lowerName2.assign(jDDLDef.structs[i].name);
                        utils::MakeStringLowerCase(lowerName2);
                        if (lowerName2 == lowerName1)
                            nameRedefinitions++;

                        if (nameRedefinitions > 1)
                            return false;
                    }

                    for (auto i = 0u; i < jDDLDef.enums.size(); i++)
                    {
                        lowerName2.assign(jDDLDef.enums[i].name);
                        utils::MakeStringLowerCase(lowerName2);
                        if (lowerName2 == lowerName1)
                            nameRedefinitions++;

                        if (nameRedefinitions > 1)
                            return false;
                    }
                    return true;
                }

                class Member
                {
                private:
                    static bool Name(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMemberDef)
                    {

                    }

                    static bool Type(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMemberDef)
                    {
                        const auto typeEnum = NameToType(jDDLMemberDef.type);
                        if (typeEnum <= DDL_STRING_TYPE)
                            return true;
                        if (typeEnum < DDL_TYPE_COUNT)
                            return false;

                        std::string lowerName1(jDDLMemberDef.name);
                        utils::MakeStringLowerCase(lowerName1);
                        std::string lowerName2;
                        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
                        {
                            lowerName2.assign(jDDLDef.structs[i].name);
                            if (lowerName1 != lowerName2)
                                continue;

                            return true;
                        }

                        return false;
                    }

                    static bool Permission(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMemberDef)
                    {
                        if (jDDLMemberDef.permission.empty())
                            return true;

                        const auto typeEnum = NameToPermissionType(jDDLMemberDef.permission);
                        if (typeEnum <= DDL_PERM_UNSPECIFIED || typeEnum >= DDL_PERM_COUNT)
                            return false;

                        return true;
                    }

                    static bool Array(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMemberDef)
                    {
                        if (!jDDLMemberDef.arrayCount.has_value())
                            return true;
                        if (jDDLMemberDef.enum_.has_value())
                            return false;
                        if (jDDLMemberDef.arrayCount.value() <= 0)
                            return false;

                        return true;
                    }

                    static bool MaxCharacters(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMemberDef)
                    {
                        const auto isStringType = NameToType(jDDLMemberDef.type) == DDL_STRING_TYPE;
                        if (isStringType)
                        {
                            if (!jDDLMemberDef.maxCharacters.has_value())
                                return false;
                            if (jDDLMemberDef.maxCharacters.value() <= 0)
                                return false;

                            return true;
                        }
                        else if (!jDDLMemberDef.maxCharacters.has_value())
                            return true;

                        return false;
                    }

                    static bool Enum_(const JsonDDLDef& jDDLDef, const JsonDDLMemberDef& jDDLMemberDef)
                    {
                        if (!jDDLMemberDef.enum_.has_value())
                            return true;
                        if (jDDLMemberDef.arrayCount.has_value())
                            return false;

                        std::string lowerName1(jDDLMemberDef.name);
                        utils::MakeStringLowerCase(lowerName1);
                        std::string lowerName2;
                        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
                        {
                            lowerName2.assign(jDDLDef.enums[i].name);
                            if (lowerName1 != lowerName2)
                                continue;

                            return true;
                        }

                        return false;
                    }
                };
            };

            class Enum
            {
            private:
                static bool Name(const JsonDDLDef& jDDLDef, const JsonDDLEnumDef& jDDLEnumDef)
                {
                    return true;
                }

                static bool Members(const JsonDDLDef& jDDLDef, const JsonDDLEnumDef& jDDLEnumDef)
                {
                    if (jDDLEnumDef.members.empty())
                        return false;
                    return true;
                }
            };
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

        bool CreateDDLDefFromJson(const JsonDDLDef& jDDLDef, ddlDef_t& ddlDef) const
        {
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

        bool CreateDDLRootFromJson(const JsonDDLRoot& jDDLRoot, ddlRoot_t& ddlRoot, ISearchPath& searchPath) const
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
                if (!LoadDDLDefJson(jDDLRoot.defFiles[i], searchPath, jDDLDef))
                    return false;

                jDDLDefs.push_back(jDDLDef);
            }

            for (auto i = 0u; i < jDDLDefs.size(); i++)
            {
                jDDLDefs[i]
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
