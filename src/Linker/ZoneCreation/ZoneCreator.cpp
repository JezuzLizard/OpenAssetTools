#include "ZoneCreator.h"

#include "AssetLoading/AssetLoadingContext.h"
#include "IObjCompiler.h"
#include "IObjLoader.h"

#include <cassert>

namespace
{
    std::unique_ptr<Zone> CreateZone(const ZoneCreationContext& context, const GameId gameId)
    {
        return std::make_unique<Zone>(context.m_definition->m_name, 0, IGame::GetGameById(gameId));
    }

    void InitializeAssetPools(Zone& zone, const GameId gameId)
    {
        zone.m_pools = ZoneAssetPools::CreateForGame(gameId, &zone, zone.m_priority);
        const auto assetTypeCount = zone.m_pools->GetAssetTypeCount();
        for (auto assetType = 0; assetType < assetTypeCount; assetType++)
            zone.m_pools->InitPoolDynamic(assetType);
    }

    std::vector<Gdt*> CreateGdtList(const ZoneCreationContext& context)
    {
        std::vector<Gdt*> gdtList;
        gdtList.reserve(context.m_gdt_files.size());
        for (const auto& gdt : context.m_gdt_files)
            gdtList.push_back(gdt.get());

        return gdtList;
    }

    void IgnoreReferencesFromAssets(ZoneCreationContext& context)
    {
        for (const auto& assetEntry : context.m_definition->m_assets)
        {
            if (!assetEntry.m_is_reference)
                continue;

            context.m_ignored_assets.m_entries.emplace_back(assetEntry.m_asset_type, assetEntry.m_asset_name, assetEntry.m_is_reference);
        }
    }
} // namespace

namespace zone_creator
{
    std::unique_ptr<Zone> CreateZoneForDefinition(GameId gameId, ZoneCreationContext& context)
    {
        auto zone = CreateZone(context, gameId);
        InitializeAssetPools(*zone, gameId);

        IgnoreReferencesFromAssets(context);
        IgnoredAssetLookup ignoredAssetLookup(context.m_ignored_assets);

        const auto* objCompiler = IObjCompiler::GetObjCompilerForGame(gameId);
        const auto* objLoader = IObjLoader::GetObjLoaderForGame(gameId);

        AssetCreatorCollection creatorCollection(*zone);
        objCompiler->ConfigureCreatorCollection(creatorCollection, *zone, *context.m_definition);
        objLoader->ConfigureCreatorCollection(creatorCollection, *zone, *context.m_asset_search_path);

        AssetCreationContext creationContext(zone.get(), &creatorCollection, &ignoredAssetLookup);

        for (const auto& assetEntry : context.m_definition->m_assets)
        {
            const auto* createdAsset = creationContext.LoadDependencyGeneric(assetEntry.m_asset_type, assetEntry.m_asset_name);

            if (!createdAsset)
                return nullptr;
        }

        creatorCollection.FinalizeZone(creationContext);

        return zone;
    }
} // namespace zone_creator
