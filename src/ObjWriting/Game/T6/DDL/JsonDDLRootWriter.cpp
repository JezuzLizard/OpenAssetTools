#include "JsonDDLRootWriter.h"

#include "Game/T6/CommonT6.h"
#include "Game/T6/Json/JsonDDLRoot.h"

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

        void Dump(const ddlRoot_t* ddlRoot, AssetDumpingContext& context) const
        {
            JsonDDLRoot jsonDDLRoot;
            CreateJsonDDLRoot(jsonDDLRoot, *ddlRoot, context);

            for (auto i = 0u; i < jsonDDLRoot.defs.size(); i++)
            {
                const auto secondaryAssetFile = context.OpenAssetFile(jsonDDLRoot.defFiles[i]);
                json jDef;

                jDef["_type"] = "ddlDef";
                jDef["_game"] = "t6";
                jDef["_version"] = 1;
                jDef["def"] = jsonDDLRoot.defs[i];

                *secondaryAssetFile << std::setw(4) << jDef << "\n";
            }
            json jRoot;

            jRoot["_type"] = "ddlRoot";
            jRoot["_game"] = "t6";
            jRoot["_version"] = 1;
            jRoot["defFiles"] = jsonDDLRoot.defFiles;

            m_primaryStream << std::setw(4) << jRoot << "\n";
        }

    private:
        static const char* AssetName(const char* input)
        {
            if (input && input[0] == ',')
                return &input[1];

            return input;
        }

        static std::string NameForType(const ddlPrimitiveTypes_e& type)
        {
            if (type <= DDL_INVALID_TYPE || type >= DDL_TYPE_COUNT)
                return static_cast<int>(type) + "_unknown";

            return DDL_TYPE_NAMES[type];
        }

        static ddlPrimitiveTypes_e TypeForName(const std::string& typeName)
        {
            for (auto i = 0; i < DDL_TYPE_COUNT; i++)
            {
                if (typeName == DDL_TYPE_NAMES[i])
                    return static_cast<ddlPrimitiveTypes_e>(i);
            }

            return DDL_INVALID_TYPE;
        }

        static int GetMemberSize(const ddlMemberDef_t& ddlMemberDef)
        {
            switch (ddlMemberDef.type)
            {
            case DDL_BYTE_TYPE:
                if (ddlMemberDef.size == 8)
                    return 8;
            case DDL_SHORT_TYPE:
            case DDL_UINT_TYPE:
            case DDL_INT_TYPE:
            case DDL_UINT64_TYPE:
            case DDL_FLOAT_TYPE:
            case DDL_FIXEDPOINT_TYPE:
            case DDL_STRING_TYPE:
            case DDL_STRUCT_TYPE:
            case DDL_ENUM_TYPE:
            }
        }

        static void CreateJsonDDlMemberDef(JsonDDLMemberDef& jDDLMemberDef, const ddlMemberDef_t& ddlMemberDef)
        {
            jDDLMemberDef.name = ddlMemberDef.name;
            jDDLMemberDef.size = ddlMemberDef.size;
            jDDLMemberDef.offset = ddlMemberDef.offset;
            jDDLMemberDef.type = NameForType(static_cast<ddlPrimitiveTypes_e>(ddlMemberDef.type));
            if (ddlMemberDef.externalIndex > 0)
                jDDLMemberDef.externalIndex = ddlMemberDef.externalIndex;

            //These values being unsigned int positive limit seems to indicate it isn't applicable to the member
            if (static_cast<int>(ddlMemberDef.rangeLimit) != 0)
                jDDLMemberDef.limits.value().rangeLimit = ddlMemberDef.rangeLimit;
            if (static_cast<int>(ddlMemberDef.serverDelta) != 0)
                jDDLMemberDef.limits.value().serverDelta = ddlMemberDef.serverDelta;
            if (static_cast<int>(ddlMemberDef.clientDelta) != 0)
                jDDLMemberDef.limits.value().clientDelta = ddlMemberDef.clientDelta;

            if (ddlMemberDef.arraySize > 1) //Only required for actual arrays
                jDDLMemberDef.arraySize = ddlMemberDef.arraySize;
            if (ddlMemberDef.enumIndex > 0)
                jDDLMemberDef.enumIndex = ddlMemberDef.enumIndex;
            jDDLMemberDef.permission = ddlMemberDef.permission;
        }

        static void CreateJsonDDlStructList(JsonDDLStructDef& jDDLStructDef, const ddlStructDef_t& ddlStructDef)
        {
            jDDLStructDef.name = ddlStructDef.name;
            jDDLStructDef.members.resize(ddlStructDef.memberCount);

            for (auto i = 0; i < ddlStructDef.memberCount; i++)
                CreateJsonDDlMemberDef(jDDLStructDef.members[i], ddlStructDef.members[i]);
        }

        static void CreateJsonDDlEnumList(JsonDDLEnumDef& jDDLEnumDef, const ddlEnumDef_t& ddlEnumDef)
        {
            jDDLEnumDef.name = ddlEnumDef.name;
            jDDLEnumDef.members.resize(ddlEnumDef.memberCount);

            for (auto i = 0; i < ddlEnumDef.memberCount; i++)
                jDDLEnumDef.members[i] = ddlEnumDef.members[i];
        }

        static void CreateJsonDDlDef(JsonDDLRoot& jDDLRoot, const ddlDef_t* ddlDef, const std::filesystem::path baseFilename, AssetDumpingContext& context)
        {
            JsonDDLDef jsonDDLDef;
            jsonDDLDef.version = ddlDef->version;

            if (ddlDef->structCount > 0)
            {
                jsonDDLDef.structs.value().resize(ddlDef->structCount);
                for (auto i = 0; i < ddlDef->structCount; i++)
                    CreateJsonDDlStructList(jsonDDLDef.structs.value()[i], ddlDef->structList[i]);
            }

            if (ddlDef->enumCount > 0)
            {
                jsonDDLDef.enums.value().resize(ddlDef->enumCount);
                for (auto i = 0; i < ddlDef->enumCount; i++)
                    CreateJsonDDlEnumList(jsonDDLDef.enums.value()[i], ddlDef->enumList[i]);
            }

            auto filename = baseFilename.stem().string();
            auto extension = ".ddldef.json";
            auto parentFolder = baseFilename.parent_path().string();
            auto filenameFinal = std::format("ddl/{}/{}_version_{}{}", parentFolder, filename, jsonDDLDef.version, extension);
            jDDLRoot.defFiles.push_back(filenameFinal);
            jDDLRoot.defs.push_back(jsonDDLDef);

            if (!ddlDef->next)
                return;
            
            CreateJsonDDlDef(jDDLRoot, ddlDef->next, baseFilename, context);
        }

        static void CreateJsonDDLRoot(JsonDDLRoot& jDDLRoot, const ddlRoot_t& ddlRoot, AssetDumpingContext& context)
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
        unsigned int* spook = nullptr;
        *spook == 0;
        const JsonDumper dumper(primaryStream);
        dumper.Dump(ddlRoot, context);
    }
} // namespace T6
