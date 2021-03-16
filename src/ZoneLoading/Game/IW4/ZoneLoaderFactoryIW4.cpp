#include "ZoneLoaderFactoryIW4.h"

#include <cassert>
#include <cstring>
#include <type_traits>

#include "Game/IW4/IW4.h"

#include "Utils/ClassUtils.h"
#include "ContentLoaderIW4.h"
#include "Game/IW4/GameAssetPoolIW4.h"
#include "Game/IW4/GameIW4.h"
#include "Game/GameLanguage.h"
#include "Game/IW4/ZoneConstantsIW4.h"
#include "Loading/Processor/ProcessorAuthedBlocks.h"
#include "Loading/Processor/ProcessorCaptureData.h"
#include "Loading/Processor/ProcessorInflate.h"
#include "Loading/Steps/StepVerifyMagic.h"
#include "Loading/Steps/StepSkipBytes.h"
#include "Loading/Steps/StepVerifyFileName.h"
#include "Loading/Steps/StepLoadSignature.h"
#include "Loading/Steps/StepVerifySignature.h"
#include "Loading/Steps/StepAddProcessor.h"
#include "Loading/Steps/StepAllocXBlocks.h"
#include "Loading/Steps/StepLoadZoneContent.h"
#include "Loading/Steps/StepLoadHash.h"
#include "Loading/Steps/StepRemoveProcessor.h"
#include "Loading/Steps/StepVerifyHash.h"

using namespace IW4;

class ZoneLoaderFactory::Impl
{
    static GameLanguage GetZoneLanguage(std::string& zoneName)
    {
        return GameLanguage::LANGUAGE_NONE;
    }

    static bool CanLoad(ZoneHeader& header, bool* isSecure, bool* isOfficial)
    {
        assert(isSecure != nullptr);
        assert(isOfficial != nullptr);

        if (header.m_version != ZoneConstants::ZONE_VERSION)
        {
            return false;
        }

        if (!memcmp(header.m_magic, ZoneConstants::MAGIC_SIGNED_INFINITY_WARD, 8))
        {
            *isSecure = true;
            *isOfficial = true;
            return true;
        }

        if (!memcmp(header.m_magic, ZoneConstants::MAGIC_UNSIGNED, 8))
        {
            *isSecure = false;
            *isOfficial = true;
            return true;
        }

        return false;
    }

    static void SetupBlock(ZoneLoader* zoneLoader)
    {
#define XBLOCK_DEF(name, type) std::make_unique<XBlock>(STR(name), name, type)

        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_TEMP, XBlock::Type::BLOCK_TYPE_TEMP));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_PHYSICAL, XBlock::Type::BLOCK_TYPE_NORMAL));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_RUNTIME, XBlock::Type::BLOCK_TYPE_RUNTIME));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_VIRTUAL, XBlock::Type::BLOCK_TYPE_NORMAL));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_LARGE, XBlock::Type::BLOCK_TYPE_NORMAL));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_CALLBACK, XBlock::Type::BLOCK_TYPE_NORMAL));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_VERTEX, XBlock::Type::BLOCK_TYPE_NORMAL));
        zoneLoader->AddXBlock(XBLOCK_DEF(IW4::XFILE_BLOCK_INDEX, XBlock::Type::BLOCK_TYPE_NORMAL));

#undef XBLOCK_DEF
    }

    static std::unique_ptr<IPublicKeyAlgorithm> SetupRSA(const bool isOfficial)
    {
        if (isOfficial)
        {
            auto rsa = Crypto::CreateRSA(IPublicKeyAlgorithm::HashingAlgorithm::RSA_HASH_SHA256,
                                          Crypto::RSAPaddingMode::RSA_PADDING_PSS);

            if (!rsa->SetKey(ZoneConstants::RSA_PUBLIC_KEY_INFINITY_WARD, sizeof(ZoneConstants::RSA_PUBLIC_KEY_INFINITY_WARD)))
            {
                printf("Invalid public key for signature checking\n");
                return nullptr;
            }

            return rsa;
        }
        else
        {
            assert(false);

            // TODO: Load custom RSA key here
            return nullptr;
        }
    }

    static void AddAuthHeaderSteps(const bool isSecure, const bool isOfficial, ZoneLoader* zoneLoader,
                                   std::string& fileName)
    {
        // Unsigned zones do not have an auth header
        if (!isSecure)
            return;

        // If file is signed setup a RSA instance.
        auto rsa = SetupRSA(isOfficial);

        zoneLoader->AddLoadingStep(std::make_unique<StepVerifyMagic>(ZoneConstants::MAGIC_AUTH_HEADER));
        zoneLoader->AddLoadingStep(std::make_unique<StepSkipBytes>(4)); // Skip reserved

        auto subHeaderHash = std::make_unique<StepLoadHash>(sizeof DB_AuthHash::bytes, 1);
        auto* subHeaderHashPtr = subHeaderHash.get();
        zoneLoader->AddLoadingStep(std::move(subHeaderHash));

        auto subHeaderHashSignature = std::make_unique<StepLoadSignature>(sizeof DB_AuthSignature::bytes);
        auto* subHeaderHashSignaturePtr = subHeaderHashSignature.get();
        zoneLoader->AddLoadingStep(std::move(subHeaderHashSignature));

        zoneLoader->AddLoadingStep(std::make_unique<StepVerifySignature>(std::move(rsa), subHeaderHashSignaturePtr, subHeaderHashPtr));

        auto subHeaderCapture = std::make_unique<ProcessorCaptureData>(sizeof(DB_AuthSubHeader));
        auto* subHeaderCapturePtr = subHeaderCapture.get();
        zoneLoader->AddLoadingStep(std::make_unique<StepAddProcessor>(std::move(subHeaderCapture)));

        zoneLoader->AddLoadingStep(std::make_unique<StepVerifyFileName>(fileName, sizeof(DB_AuthSubHeader::fastfileName)));
        zoneLoader->AddLoadingStep(std::make_unique<StepSkipBytes>(4)); // Skip reserved

        auto masterBlockHashes = std::make_unique<StepLoadHash>(sizeof DB_AuthHash::bytes, std::extent<decltype(DB_AuthSubHeader::masterBlockHashes)>::value);
        auto* masterBlockHashesPtr = masterBlockHashes.get();
        zoneLoader->AddLoadingStep(std::move(masterBlockHashes));

        zoneLoader->AddLoadingStep(std::make_unique<StepRemoveProcessor>(subHeaderCapturePtr));
        zoneLoader->AddLoadingStep(std::make_unique<StepVerifyHash>(std::unique_ptr<IHashFunction>(Crypto::CreateSHA256()), 0, subHeaderHashPtr, subHeaderCapturePtr));

        // Skip the rest of the first chunk
        zoneLoader->AddLoadingStep(std::make_unique<StepSkipBytes>(ZoneConstants::AUTHED_CHUNK_SIZE - sizeof(DB_AuthHeader)));

        zoneLoader->AddLoadingStep(std::make_unique<StepAddProcessor>(std::make_unique<ProcessorAuthedBlocks>(
            ZoneConstants::AUTHED_CHUNK_COUNT_PER_GROUP, ZoneConstants::AUTHED_CHUNK_SIZE, std::extent<decltype(DB_AuthSubHeader::masterBlockHashes)>::value,
            std::unique_ptr<IHashFunction>(Crypto::CreateSHA256()), masterBlockHashesPtr)));
    }

public:
    static ZoneLoader* CreateLoaderForHeader(ZoneHeader& header, std::string& fileName)
    {
        bool isSecure;
        bool isOfficial;

        // Check if this file is a supported IW4 zone.
        if (!CanLoad(header, &isSecure, &isOfficial))
            return nullptr;

        // Create new zone
        auto* zone = new Zone(fileName, 0, &g_GameIW4);
        zone->m_pools = std::make_unique<GameAssetPoolIW4>(zone, 0);
        zone->m_language = GetZoneLanguage(fileName);

        // File is supported. Now setup all required steps for loading this file.
        auto* zoneLoader = new ZoneLoader(zone);

        SetupBlock(zoneLoader);

        // Skip unknown 1 byte field that the game ignores as well
        zoneLoader->AddLoadingStep(std::make_unique<StepSkipBytes>(1));

        // Skip timestamp
        zoneLoader->AddLoadingStep(std::make_unique<StepSkipBytes>(8));

        // Add steps for loading the auth header which also contain the signature of the zone if it is signed.
        AddAuthHeaderSteps(isSecure, isOfficial, zoneLoader, fileName);

        zoneLoader->AddLoadingStep(std::make_unique<StepAddProcessor>(std::make_unique<ProcessorInflate>(ZoneConstants::AUTHED_CHUNK_SIZE)));

        // Start of the XFile struct
        zoneLoader->AddLoadingStep(std::make_unique<StepSkipBytes>(8));
        // Skip size and externalSize fields since they are not interesting for us
        zoneLoader->AddLoadingStep(std::make_unique<StepAllocXBlocks>());

        // Start of the zone content
        zoneLoader->AddLoadingStep(std::make_unique<StepLoadZoneContent>(std::make_unique<ContentLoader>(), zone, ZoneConstants::OFFSET_BLOCK_BIT_COUNT, ZoneConstants::INSERT_BLOCK));

        // Return the fully setup zoneloader
        return zoneLoader;
    }
};

ZoneLoader* ZoneLoaderFactory::CreateLoaderForHeader(ZoneHeader& header, std::string& fileName)
{
    return Impl::CreateLoaderForHeader(header, fileName);
}
