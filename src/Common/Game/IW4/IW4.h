#pragma once

//#include <d3d11.h>
#include "Image/Texture.h"

#include "IW4_Assets.h"

namespace IW4
{
    struct DB_AuthHash
    {
        char bytes[32];
    };

    struct DB_AuthSignature
    {
        char bytes[256];
    };

    struct DB_AuthSubHeader
    {
        char fastfileName[32];
        unsigned int reserved;
        DB_AuthHash masterBlockHashes[244];
    };

    struct DB_AuthHeader
    {
        char magic[8];                                  // + 0
        unsigned int reserved;                          // + 8
        DB_AuthHash subheaderHash;                      // + 12
        DB_AuthSignature signedSubheaderHash;           // + 44
        DB_AuthSubHeader subheader;                     // + 300
    };

    struct ScriptStringList
    {
        int count;
        const char** strings;
    };

    struct XAsset
    {
        XAssetType type;
        XAssetHeader header;
    };

    struct XAssetList
    {
        ScriptStringList stringList;
        int assetCount;
        XAsset* assets;
    };

    struct cspField_t
    {
        const char* szName;
        int iOffset;
        int iFieldType;
    };

    enum csParseFieldType_t
    {
        CSPFT_STRING = 0,
        CSPFT_STRING_MAX_STRING_CHARS,
        CSPFT_STRING_MAX_QPATH,
        CSPFT_STRING_MAX_OSPATH,
        CSPFT_INT,
        CSPFT_QBOOLEAN,
        CSPFT_BOOL,
        CSPFT_FLOAT,
        CSPFT_MPH_TO_INCHES_PER_SEC,
        CSPFT_MILLISECONDS,
        CSPFT_FX,
        CSPFT_XMODEL,
        CSPFT_MATERIAL,
        CSPFT_PHYS_COLLMAP,
        CSPFT_SOUND,
        CSPFT_TRACER,

        CSPFT_NUM_BASE_FIELD_TYPES,
    };

    enum weapFieldType_t
    {
        WFT_WEAPONTYPE = CSPFT_NUM_BASE_FIELD_TYPES,
        WFT_WEAPONCLASS,
        WFT_OVERLAYRETICLE,
        WFT_PENETRATE_TYPE,
        WFT_IMPACT_TYPE,
        WFT_STANCE,
        WFT_PROJ_EXPLOSION,
        WFT_OFFHAND_CLASS,
        WFT_ANIMTYPE,
        WFT_ACTIVE_RETICLE_TYPE,
        WFT_GUIDED_MISSILE_TYPE,
        WFT_BOUNCE_SOUND,
        WFT_STICKINESS,
        WFT_OVERLAYINTERFACE,
        WFT_INVENTORYTYPE,
        WFT_FIRETYPE,
        WFT_AMMOCOUNTER_CLIPTYPE,
        WFT_ICONRATIO_HUD,
        WFT_ICONRATIO_PICKUP,
        WFT_ICONRATIO_AMMOCOUNTER,
        WFT_ICONRATIO_KILL,
        WFT_ICONRATIO_DPAD,
        WFT_HIDETAGS,
        WFT_NOTETRACKSOUNDMAP,
        WFT_NOTETRACKRUMBLEMAP,

        WFT_NUM_FIELD_TYPES,
    };

    enum VehicleFieldType
    {
        VFT_TYPE = CSPFT_NUM_BASE_FIELD_TYPES,
        VFT_AXLE_STEERING,
        VFT_AXLE_POWER,
        VFT_AXLE_BRAKING,
        VFT_TROPHY_TAGS,

        VFT_NUM,
    };
}