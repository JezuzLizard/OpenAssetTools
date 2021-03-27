#pragma once
#include "Game/IGame.h"

class GameT6 : public IGame
{
    std::vector<Zone*> m_zones;

public:
    std::string GetFullName() override;
    std::string GetShortName() override;
    void AddZone(Zone* zone) override;
    void RemoveZone(Zone* zone) override;
    std::vector<Zone*> GetZones() override;
    std::vector<GameLanguagePrefix> GetLanguagePrefixes() override;
};

extern GameT6 g_GameT6;