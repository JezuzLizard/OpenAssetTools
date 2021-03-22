#include "ZoneScriptStrings.h"

#include <stdexcept>
#include <sstream>

void ZoneScriptStrings::AddScriptString(const std::string& value)
{
    const auto newScrStringIndex = static_cast<scr_string_t>(m_scr_strings.size());
    m_scr_strings.emplace_back(value);
    m_scr_string_lookup[value] = newScrStringIndex;
}

scr_string_t ZoneScriptStrings::AddOrGetScriptString(const std::string& value)
{
    if (m_scr_strings.empty())
    {
        m_scr_strings.emplace_back("");
        m_scr_string_lookup[""] = static_cast<scr_string_t>(0);
    }
    else
    {
        const auto existingScriptString = m_scr_string_lookup.find(value);
        if (existingScriptString != m_scr_string_lookup.end())
            return existingScriptString->second;
    }

    const auto newScrStringIndex = static_cast<scr_string_t>(m_scr_strings.size());
    m_scr_strings.emplace_back(value);
    m_scr_string_lookup[value] = newScrStringIndex;

    return newScrStringIndex;
}

scr_string_t ZoneScriptStrings::GetScriptString(const std::string& value) const
{
    const auto existingScriptString = m_scr_string_lookup.find(value);
    if (existingScriptString != m_scr_string_lookup.end())
        return existingScriptString->second;

    std::ostringstream str;
    str << "Unable to find script string \"" << value << "\"";
    throw std::runtime_error(str.str());
}

size_t ZoneScriptStrings::Count() const
{
    return m_scr_strings.size();
}

bool ZoneScriptStrings::Empty() const
{
    return m_scr_strings.empty();
}

const std::string& ZoneScriptStrings::operator[](const size_t index) const
{
    if(index > m_scr_strings.size())
    {
        std::ostringstream str;
        str << "Script string index '" << index << "' is not inside range of zone script strings (count: " << m_scr_strings.size() << ")";
        throw std::runtime_error(str.str());
    }

    return m_scr_strings[index];
}

std::vector<std::string>::const_iterator ZoneScriptStrings::begin() const
{
    return m_scr_strings.cbegin();
}

std::vector<std::string>::const_iterator ZoneScriptStrings::end() const
{
    return m_scr_strings.end();
}
