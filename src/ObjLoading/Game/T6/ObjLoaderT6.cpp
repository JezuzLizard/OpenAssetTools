#include "ObjLoaderT6.h"

#include "Asset/GlobalAssetPoolsLoader.h"
#include "AssetLoading/AssetLoadingManager.h"
#include "Game/T6/CommonT6.h"
#include "Game/T6/GameAssetPoolT6.h"
#include "Game/T6/GameT6.h"
#include "Game/T6/T6.h"
#include "Image/Dx12TextureLoader.h"
#include "Image/IwiLoader.h"
#include "Image/IwiTypes.h"
#include "Image/Texture.h"
#include "Localize/AssetLoaderLocalizeT6.h"
#include "ObjContainer/IPak/IPak.h"
#include "ObjLoading.h"

#include <format>
#include <memory>

namespace T6
{
    constexpr auto IPAK_READ_HASH = Common::Com_HashKey("ipak_read", 64);
    constexpr auto GLOBAL_HASH = Common::Com_HashKey("GLOBAL", 64);

    bool ObjLoader::VerifySoundBankChecksum(const SoundBank& soundBank, const SndRuntimeAssetBank& sndRuntimeAssetBank)
    {
        SoundAssetBankChecksum checksum{};
        static_assert(sizeof(SoundAssetBankChecksum::checksumBytes) == sizeof(SndRuntimeAssetBank::linkTimeChecksum));
        for (auto i = 0u; i < sizeof(SoundAssetBankChecksum::checksumBytes); i++)
            checksum.checksumBytes[i] = sndRuntimeAssetBank.linkTimeChecksum[i];

        return soundBank.VerifyChecksum(checksum);
    }

    SoundBank* ObjLoader::LoadSoundBankForZone(ISearchPath& searchPath, const std::string& soundBankFileName, Zone& zone)
    {
        if (ObjLoading::Configuration.Verbose)
            std::cout << std::format("Trying to load sound bank '{}' for zone '{}'\n", soundBankFileName, zone.m_name);

        auto* existingSoundBank = SoundBank::Repository.GetContainerByName(soundBankFileName);
        if (existingSoundBank != nullptr)
        {
            if (ObjLoading::Configuration.Verbose)
                std::cout << std::format("Referencing loaded sound bank '{}'.\n", soundBankFileName);

            SoundBank::Repository.AddContainerReference(existingSoundBank, &zone);
            return existingSoundBank;
        }

        auto file = searchPath.Open(soundBankFileName);
        if (file.IsOpen())
        {
            auto sndBank = std::make_unique<SoundBank>(soundBankFileName, std::move(file.m_stream), file.m_length);
            auto* sndBankPtr = sndBank.get();

            if (!sndBank->Initialize())
            {
                std::cerr << std::format("Failed to load sound bank '{}'\n", soundBankFileName);
                return nullptr;
            }

            SoundBank::Repository.AddContainer(std::move(sndBank), &zone);

            if (ObjLoading::Configuration.Verbose)
                std::cout << std::format("Found and loaded sound bank '{}'\n", soundBankFileName);

            return sndBankPtr;
        }

        std::cerr << std::format("Failed to load sound bank '{}'\n", soundBankFileName);
        return nullptr;
    }

    void ObjLoader::LoadSoundBankFromLinkedInfo(ISearchPath& searchPath,
                                                const std::string& soundBankFileName,
                                                const SndRuntimeAssetBank& sndBankLinkedInfo,
                                                Zone& zone,
                                                std::set<std::string>& loadedBanksForZone,
                                                std::stack<std::string>& dependenciesToLoad)
    {
        if (loadedBanksForZone.find(soundBankFileName) == loadedBanksForZone.end())
        {
            auto* soundBank = LoadSoundBankForZone(searchPath, soundBankFileName, zone);

            if (soundBank)
            {
                if (!VerifySoundBankChecksum(*soundBank, sndBankLinkedInfo))
                    std::cout << std::format("Checksum of sound bank does not match link time checksum for '{}'\n", soundBankFileName);

                loadedBanksForZone.emplace(soundBankFileName);

                for (const auto& dependency : soundBank->GetDependencies())
                {
                    dependenciesToLoad.emplace(dependency);
                }
            }
        }
    }

    void ObjLoader::LoadSoundBanksFromAsset(ISearchPath& searchPath, const SndBank& sndBank, Zone& zone, std::set<std::string>& loadedBanksForZone)
    {
        std::stack<std::string> dependenciesToLoad;

        if (sndBank.streamAssetBank.zone)
        {
            const auto soundBankFileName = SoundBank::GetFileNameForDefinition(true, sndBank.streamAssetBank.zone, sndBank.streamAssetBank.language);
            LoadSoundBankFromLinkedInfo(searchPath, soundBankFileName, sndBank.streamAssetBank, zone, loadedBanksForZone, dependenciesToLoad);
        }

        if (sndBank.runtimeAssetLoad && sndBank.loadAssetBank.zone)
        {
            const auto soundBankFileName = SoundBank::GetFileNameForDefinition(false, sndBank.loadAssetBank.zone, sndBank.loadAssetBank.language);
            LoadSoundBankFromLinkedInfo(searchPath, soundBankFileName, sndBank.loadAssetBank, zone, loadedBanksForZone, dependenciesToLoad);
        }

        while (!dependenciesToLoad.empty())
        {
            auto dependencyFileName = dependenciesToLoad.top();
            dependenciesToLoad.pop();

            if (loadedBanksForZone.find(dependencyFileName) == loadedBanksForZone.end())
            {
                auto* soundBank = LoadSoundBankForZone(searchPath, dependencyFileName, zone);

                if (soundBank)
                {
                    loadedBanksForZone.emplace(dependencyFileName);

                    for (const auto& dependency : soundBank->GetDependencies())
                        dependenciesToLoad.emplace(dependency);
                }
            }
        }
    }

    void ObjLoader::LoadIPakForZone(ISearchPath& searchPath, const std::string& ipakName, Zone& zone)
    {
        if (ObjLoading::Configuration.Verbose)
            std::cout << std::format("Trying to load ipak '{}' for zone '{}'\n", ipakName, zone.m_name);

        auto* existingIPak = IPak::Repository.GetContainerByName(ipakName);
        if (existingIPak != nullptr)
        {
            if (ObjLoading::Configuration.Verbose)
                std::cout << std::format("Referencing loaded ipak '{}'.\n", ipakName);

            IPak::Repository.AddContainerReference(existingIPak, &zone);
            return;
        }

        const auto ipakFilename = std::format("{}.ipak", ipakName);

        auto file = searchPath.Open(ipakFilename);
        if (file.IsOpen())
        {
            auto ipak = std::make_unique<IPak>(ipakFilename, std::move(file.m_stream));

            if (ipak->Initialize())
            {
                IPak::Repository.AddContainer(std::move(ipak), &zone);

                if (ObjLoading::Configuration.Verbose)
                    std::cout << std::format("Found and loaded ipak '{}'.\n", ipakFilename);
            }
            else
            {
                std::cerr << std::format("Failed to load ipak '{}'!\n", ipakFilename);
            }
        }
    }

    bool ObjLoader::IsMpZone(const Zone& zone)
    {
        return zone.m_name.compare(0, 3, "mp_") == 0 || zone.m_name.compare(zone.m_name.length() - 3, 3, "_mp") == 0;
    }

    bool ObjLoader::IsZmZone(const Zone& zone)
    {
        return zone.m_name.compare(0, 3, "zm_") == 0 || zone.m_name.compare(zone.m_name.length() - 3, 3, "_zm") == 0;
    }

    void ObjLoader::LoadCommonIPaks(ISearchPath& searchPath, Zone& zone)
    {
        if (ObjLoading::Configuration.Verbose)
            std::cout << std::format("Loading common ipaks for zone \"{}\"\n", zone.m_name);

        LoadIPakForZone(searchPath, "base", zone);
        const auto& languagePrefixes = IGame::GetGameById(GameId::T6)->GetLanguagePrefixes();
        for (const auto& languagePrefix : languagePrefixes)
            LoadIPakForZone(searchPath, std::format("{}base", languagePrefix.m_prefix), zone);

        if (IsMpZone(zone))
        {
            if (ObjLoading::Configuration.Verbose)
                std::cout << std::format("Loading multiplayer ipaks for zone \"{}\"\n", zone.m_name);

            LoadIPakForZone(searchPath, "mp", zone);
            LoadIPakForZone(searchPath, "so", zone);
        }
        else if (IsZmZone(zone))
        {
            if (ObjLoading::Configuration.Verbose)
                std::cout << std::format("Loading zombie ipak for zone \"{}\"\n", zone.m_name);

            LoadIPakForZone(searchPath, "zm", zone);
        }
        else
        {
            if (ObjLoading::Configuration.Verbose)
                std::cout << std::format("Loading singleplayer ipak for zone \"{}\"\n", zone.m_name);

            LoadIPakForZone(searchPath, "sp", zone);
        }
    }

    void ObjLoader::LoadReferencedContainersForZone(ISearchPath& searchPath, Zone& zone) const
    {
        const auto* assetPoolT6 = dynamic_cast<GameAssetPoolT6*>(zone.m_pools.get());
        const auto zoneNameHash = Common::Com_HashKey(zone.m_name.c_str(), 64);

        LoadCommonIPaks(searchPath, zone);

        if (assetPoolT6->m_key_value_pairs != nullptr)
        {
            for (auto* keyValuePairsEntry : *assetPoolT6->m_key_value_pairs)
            {
                const auto* keyValuePairs = keyValuePairsEntry->Asset();
                for (auto variableIndex = 0; variableIndex < keyValuePairs->numVariables; variableIndex++)
                {
                    auto* variable = &keyValuePairs->keyValuePairs[variableIndex];

                    if (variable->namespaceHash == zoneNameHash && variable->keyHash == IPAK_READ_HASH)
                    {
                        LoadIPakForZone(searchPath, variable->value, zone);
                    }
                }
            }
        }

        if (assetPoolT6->m_sound_bank != nullptr)
        {
            std::set<std::string> loadedSoundBanksForZone;

            for (auto* sndBankAssetInfo : *assetPoolT6->m_sound_bank)
            {
                LoadSoundBanksFromAsset(searchPath, *sndBankAssetInfo->Asset(), zone, loadedSoundBanksForZone);
            }
        }
    }

    void ObjLoader::UnloadContainersOfZone(Zone& zone) const
    {
        IPak::Repository.RemoveContainerReferences(&zone);
    }

    namespace
    {
        void ConfigureDefaultCreators(AssetCreatorCollection& collection, Zone& zone)
        {
            auto& memory = *zone.GetMemory();
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorPhysPreset>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorPhysConstraints>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorDestructibleDef>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorXAnim>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorXModel>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorMaterial>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorTechniqueSet>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorImage>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorSoundBank>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorSoundPatch>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorClipMap>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorClipMapPvs>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorComWorld>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorGameWorldSp>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorGameWorldMp>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorMapEnts>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorGfxWorld>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorLightDef>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorFont>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorMenuList>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorMenu>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorLocalize>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorWeapon>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorSoundDriverGlobals>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorFx>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorImpactFx>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorRawFile>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorStringTable>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorPackIndex>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorXGlobals>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorDDL>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorGlasses>(memory));
            // collection.AddDefaultAssetCreator(std::make_unique<DefaultCreatorEmblemSet>(memory));
        }

        void ConfigureGlobalAssetPoolsLoaders(AssetCreatorCollection& collection, Zone& zone)
        {
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetPhysPreset>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetPhysConstraints>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetDestructibleDef>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetXAnim>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetXModel>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMaterial>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetTechniqueSet>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetImage>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSoundBank>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSoundPatch>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetClipMap>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetClipMapPvs>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetComWorld>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGameWorldSp>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGameWorldMp>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMapEnts>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGfxWorld>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLightDef>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFont>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFontIcon>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMenuList>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMenu>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLocalize>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetWeapon>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetAttachment>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetAttachmentUnique>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetWeaponCamo>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSoundDriverGlobals>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFx>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetImpactFx>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetRawFile>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetStringTable>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLeaderboard>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetXGlobals>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetDDL>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGlasses>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetEmblemSet>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetScript>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetVehicle>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMemoryBlock>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetAddonMapEnts>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetTracer>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSkinnedVerts>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetQdb>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSlug>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFootstepTable>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFootstepFxTable>>(zone));
            collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetZBarrier>>(zone));
        }

        void ConfigureDefaultCreators(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath)
        {
            auto& memory = *zone.GetMemory();
            // collection.AddAssetCreator(std::make_unique<AssetLoaderPhysPreset>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderPhysConstraints>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderDestructibleDef>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderXAnim>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderXModel>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderMaterial>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderTechniqueSet>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderImage>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderSoundBank>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderSoundPatch>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderClipMap>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderClipMapPvs>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderComWorld>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderGameWorldSp>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderGameWorldMp>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderMapEnts>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderGfxWorld>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderLightDef>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderFont>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderFontIcon>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderMenuList>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderMenu>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderLocalize>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderWeapon>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderAttachment>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderAttachmentUnique>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderWeaponCamo>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderSoundDriverGlobals>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderFx>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderImpactFx>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderRawFile>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderStringTable>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderLeaderboard>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderXGlobals>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderDDL>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderGlasses>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderEmblemSet>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderScript>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderVehicle>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderMemoryBlock>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderAddonMapEnts>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderTracer>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderSkinnedVerts>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderQdb>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderSlug>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderFootstepTable>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderFootstepFxTable>(memory));
            // collection.AddAssetCreator(std::make_unique<AssetLoaderZBarrier>(memory));
        }
    } // namespace

    void ObjLoader::ConfigureCreatorCollection(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath) const
    {
        ConfigureDefaultCreators(collection, zone, searchPath);
        ConfigureGlobalAssetPoolsLoaders(collection, zone);
    }
} // namespace T6
