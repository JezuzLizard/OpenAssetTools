#include "InfoStringToStructConverterBase.h"

#include <cstring>
#include <iostream>
#include <sstream>

InfoStringToStructConverterBase::InfoStringToStructConverterBase(const InfoString& infoString,
                                                                 void* structure,
                                                                 ZoneScriptStrings& zoneScriptStrings,
                                                                 MemoryManager* memory)
    : m_info_string(infoString),
      m_zone_script_strings(zoneScriptStrings),
      m_memory(memory),
      m_structure(structure)
{
}

bool InfoStringToStructConverterBase::ParseAsArray(const std::string& value, std::vector<std::string>& valueArray)
{
    auto startPos = 0u;
    for (auto ci = 0u; ci < value.size(); ci++)
    {
        const auto c = value[ci];

        if (c == '\r' && ci + 1 < value.size() && value[ci + 1] == '\n')
        {
            valueArray.emplace_back(value, startPos, ci - startPos);
            startPos = ++ci + 1;
        }
        else if (c == '\n')
        {
            valueArray.emplace_back(value, startPos, ci - startPos);
            startPos = ci + 1;
        }
    }

    if (startPos < value.size())
    {
        valueArray.emplace_back(value, startPos, value.size() - startPos);
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertString(const std::string& value, const size_t offset)
{
    *reinterpret_cast<const char**>(reinterpret_cast<uintptr_t>(m_structure) + offset) = m_memory->Dup(value.c_str());
    return true;
}

bool InfoStringToStructConverterBase::ConvertStringBuffer(const std::string& value, const size_t offset, const size_t bufferSize)
{
    strncpy(reinterpret_cast<char*>(reinterpret_cast<uintptr_t>(m_structure) + offset), value.c_str(), bufferSize);
    return true;
}

bool InfoStringToStructConverterBase::ConvertInt(const std::string& value, const size_t offset)
{
    char* endPtr;
    *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = strtol(value.c_str(), &endPtr, 0);

    if (endPtr != &value[value.size()])
    {
        std::cout << "Failed to parse value \"" << value << "\" as int\n";
        return false;
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertUint(const std::string& value, const size_t offset)
{
    char* endPtr;
    *reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = strtoul(value.c_str(), &endPtr, 0);

    if (endPtr != &value[value.size()])
    {
        std::cout << "Failed to parse value \"" << value << "\" as uint\n";
        return false;
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertBool(const std::string& value, const size_t offset)
{
    char* endPtr;
    const auto intValue = strtol(value.c_str(), &endPtr, 0);

    *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = intValue != 0;
    if (endPtr != &value[value.size()])
    {
        std::cout << "Failed to parse value \"" << value << "\" as bool\n";
        return false;
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertQBoolean(const std::string& value, const size_t offset)
{
    char* endPtr;
    const auto intValue = strtol(value.c_str(), &endPtr, 0);

    *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = intValue != 0 ? 1 : 0;
    if (endPtr != &value[value.size()])
    {
        std::cout << "Failed to parse value \"" << value << "\" as qboolean\n";
        return false;
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertFloat(const std::string& value, const size_t offset)
{
    char* endPtr;
    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = strtof(value.c_str(), &endPtr);

    if (endPtr != &value[value.size()])
    {
        std::cout << "Failed to parse value \"" << value << "\" as float\n";
        return false;
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertMilliseconds(const std::string& value, const size_t offset)
{
    char* endPtr;
    *reinterpret_cast<unsigned int*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = static_cast<unsigned int>(strtof(value.c_str(), &endPtr) * 1000.0f);

    if (endPtr != &value[value.size()])
    {
        std::cout << "Failed to parse value \"" << value << "\" as milliseconds\n";
        return false;
    }

    return true;
}

bool InfoStringToStructConverterBase::ConvertScriptString(const std::string& value, const size_t offset)
{
    auto scrStrValue = m_zone_script_strings.AddOrGetScriptString(value);
    m_used_script_string_list.emplace(scrStrValue);
    *reinterpret_cast<scr_string_t*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = scrStrValue;

    return true;
}

bool InfoStringToStructConverterBase::ConvertEnumInt(
    const std::string& fieldName, const std::string& value, const size_t offset, const char** enumValues, const size_t enumSize)
{
    for (auto i = 0u; i < enumSize; i++)
    {
        if (value == enumValues[i])
        {
            *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_structure) + offset) = static_cast<int>(i);
            return true;
        }
    }

    std::ostringstream ss;
    ss << "Not a valid value for field \"" << fieldName << "\": \"" << value << "\". Valid values are:\n    ";

    for (auto i = 0u; i < enumSize; i++)
    {
        if (i > 0)
            ss << ", ";
        ss << '"' << enumValues[i] << '"';
    }

    ss << '\n';
    std::cerr << ss.str();

    return false;
}

std::vector<scr_string_t> InfoStringToStructConverterBase::GetUsedScriptStrings() const
{
    std::vector<scr_string_t> scrStringList;
    scrStringList.reserve(m_used_script_string_list.size());
    for (auto scrStr : m_used_script_string_list)
    {
        scrStringList.push_back(scrStr);
    }

    return scrStringList;
}

std::vector<XAssetInfoGeneric*> InfoStringToStructConverterBase::GetDependencies() const
{
    std::vector<XAssetInfoGeneric*> dependencyList;
    dependencyList.reserve(m_dependencies.size());
    for (auto* dependency : m_dependencies)
    {
        dependencyList.push_back(dependency);
    }

    return dependencyList;
}

std::vector<IndirectAssetReference> InfoStringToStructConverterBase::GetIndirectAssetReferences() const
{
    std::vector<IndirectAssetReference> indirectAssetReferences;
    indirectAssetReferences.reserve(m_indirect_asset_references.size());
    for (auto& assetReference : m_indirect_asset_references)
    {
        indirectAssetReferences.emplace_back(assetReference);
    }

    return indirectAssetReferences;
}
