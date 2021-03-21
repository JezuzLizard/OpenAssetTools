#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

class ZoneDefinitionEntry
{
public:
    std::string m_asset_type;
    std::string m_asset_name;
    bool m_is_reference;

    ZoneDefinitionEntry();
    ZoneDefinitionEntry(std::string type, std::string name, bool isReference);
};

class ZoneMetaDataEntry
{
public:
    std::string m_key;
    std::string m_value;

    ZoneMetaDataEntry();
    ZoneMetaDataEntry(std::string key, std::string value);
};

class ZoneDefinition
{
public:
    std::vector<std::unique_ptr<ZoneMetaDataEntry>> m_metadata;
    std::unordered_multimap<std::string, ZoneMetaDataEntry*> m_metadata_lookup;
    std::vector<std::string> m_includes;
    std::vector<std::string> m_ignores;
    std::vector<ZoneDefinitionEntry> m_assets;

    void AddMetaData(std::string key, std::string value);
    void Include(ZoneDefinition& definitionToInclude);
};