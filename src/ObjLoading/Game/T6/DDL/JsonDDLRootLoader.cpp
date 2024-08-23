#include "JsonDDLRootLoader.h"

#include "Game/T6/CommonT6.h"
#include "Game/T6/Json/JsonDDLRoot.h"

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
        JsonLoader(std::istream& stream, MemoryManager& memory, IAssetLoadingManager& manager, std::vector<XAssetInfoGeneric*>& dependencies)
            : m_stream(stream),
              m_memory(memory),
              m_manager(manager),
              m_dependencies(dependencies)

        {
        }

        bool Load(ddlRoot_t& ddlRoot) const
        {
            return false;
        }

    private:
        std::istream& m_stream;
        MemoryManager& m_memory;
        IAssetLoadingManager& m_manager;
        std::vector<XAssetInfoGeneric*>& m_dependencies;
    };
} // namespace

namespace T6
{
    bool LoadDDLRootAsJson(
        std::istream& stream, ddlRoot_t& ddlRoot, MemoryManager* memory, IAssetLoadingManager* manager, std::vector<XAssetInfoGeneric*>& dependencies)
    {
        const JsonLoader loader(stream, *memory, *manager, dependencies);

        return loader.Load(ddlRoot);
    }
} // namespace T6
