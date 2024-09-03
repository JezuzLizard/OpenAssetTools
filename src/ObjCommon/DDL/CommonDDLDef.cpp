#include "CommonDDL.h"

CommonDDLDef::CommonDDLDef(const int version, const std::string filename)
    : m_version(version),
      m_size(0u),
      m_filename(std::move(filename))
{
}

std::optional<CommonDDLStructDef&> CommonDDLDef::FindStruct(const DDLString& name, bool checkIncludes)
{
    for (auto& struc : m_structs)
    {
        if (struc.m_name.GetLowerConst() == "root")
            continue;
        if (struc.m_name.GetLowerConst() == name.GetLowerConst())
            return struc;
    }

    if (checkIncludes)
    {
        for (auto& include : GetIncludes())
        {
            for (auto& struc : include.m_structs)
            {
                if (struc.m_name.GetLowerConst() == "root")
                    continue;
                if (struc.m_name.GetLowerConst() == name.GetLowerConst())
                    return struc;
            }
        }
    }
    return std::nullopt;
}

std::optional<CommonDDLEnumDef&> CommonDDLDef::FindEnum(const DDLString& name, bool checkIncludes)
{
    for (auto& enum_ : m_enums)
    {
        if (enum_.m_name.GetLowerConst() == "root")
            continue;
        if (enum_.m_name.GetLowerConst() == name.GetLowerConst())
            return enum_;
    }

    if (checkIncludes)
    {
        for (auto& include : GetIncludes())
        {
            for (auto& enum_ : include.m_enums)
            {
                if (enum_.m_name.GetLowerConst() == "root")
                    continue;
                if (enum_.m_name.GetLowerConst() == name.GetLowerConst())
                    return enum_;
            }
        }
    }
    return std::nullopt;
}

std::vector<CommonDDLInclude>& CommonDDLDef::GetIncludes()
{
    return m_includes;
}

void CommonDDLDef::LogicError(const std::string& message) const
{
    assert(!m_filename.empty());

    std::string prefaceAndMessage = std::format("DDL Def: {} ", m_filename) + message;
#ifdef DDL_DEBUG
    this;
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

size_t CommonDDLDef::TypeToStructIndex(const DDLString& typeName) const noexcept
{
    for (auto i = 0u; i < m_structs.size(); i++)
    {
        if (m_structs[i].m_name.GetLowerConst() == typeName.GetLowerConst())
            return i;
    }

    return 0;
}

int CommonDDLDef::TypeToEnumIndex(const DDLString& typeName) const noexcept
{
    for (auto i = 0u; i < m_enums.size(); i++)
    {
        if (m_enums[i].m_name.GetLowerConst() == typeName.GetLowerConst())
            return i;
    }

    return -1;
}

std::optional<DDLString> CommonDDLDef::StructIndexToType(const size_t index) const noexcept
{
    if (index < m_structs.size())
        return m_structs[index].m_name;

    return std::nullopt;
}

std::optional<DDLString> CommonDDLDef::EnumIndexToType(const int index) const noexcept
{
    if (index > -1 && index < m_enums.size())
        return m_enums[index].m_name;

    return std::nullopt;
}

bool CommonDDLDef::Validate() const
{
    try
    {
        ValidateRoot();
        return true;
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}

void CommonDDLDef::PreCalculate()
{
    for (auto& struc : m_structs)
    {
        struc.ResetCalculated();
        for (auto& member : struc.m_members)
            member.ResetCalculated();
    }
    for (auto& enum_ : m_enums)
        enum_.ResetCalculated();

    m_in_calculation.clear();
    m_in_calculation.resize(m_structs.size());
}

bool CommonDDLDef::Calculate()
{
    PreCalculate();

    try
    {
        for (auto& struc : m_structs)
        {
            if (struc.m_name.GetLowerConst() == "root")
            {
                struc.Calculate();
                break;
            }
        }

        for (auto& struc : m_structs)
            // Structs can canonically be linked into the asset, but do not change the size of the ddl buffer as they are not in the root struct.
            // Treyarch likely had an include system that would paste structs from an external file into the the defs that include.
            // Or they had an export/import system.
            if (struc.GetRefCount())
                m_size += struc.m_size;

        // Link unreferenced structs anyway
        for (auto& struc : m_structs)
            if (!struc.GetRefCount())
                struc.Calculate();

        // Link unreferenced enums anyway
        for (auto& enum_ : m_enums)
            if (!enum_.GetRefCount())
                enum_.CalculateHashes();

        for (auto& enum_ : m_enums)
            enum_.CalculateHashes();

        return true;
    }
    catch (DDL::Exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return false;
}

void CommonDDLDef::ValidateRoot() const
{
    for (auto i = 0u; i < m_structs.size(); i++)
    {
        if (m_structs[i].m_name.GetLowerConst() == "root")
        {
            m_structs[i].Validate();
            break;
        }

        LogicError("root struct entry point missing");
    }

    // Check reference counts for enums, and structs to determine unused enums/structs.
    // If a struct is unused it will still be linked into the asset, but it won't contribute to the size of the def.
    // Track, and validate these unused enums/structs so that the user can be alerted to a mistake(they wanted to use the struct they made, but forgot to
    // actually reference it in root). Treyarch canonically allowed defs to include structs/enums from common files, the evidence is that there are multiple
    // instances of the same struct appearing in multiple defs, with instances of it not having any references occuring multiple times.

    for (auto i = 0u; i < m_enums.size(); i++)
    {
        m_enums[i].ReferenceCount();
        if (!m_enums[i].GetRefCount())
            m_enums[i].Validate();
    }

    for (auto i = 0u; i < m_structs.size(); i++)
    {
        m_structs[i].ReferenceCount();
        if (!m_structs[i].GetRefCount())
            m_structs[i].Validate();
    }
}

void CommonDDLDef::NameError(const std::string& message) const
{
    std::string prefaceAndMessage = std::format("File: {} ", this->m_filename) + message;
#ifdef DDL_DEBUG
    __debugbreak();
#endif
    throw DDL::Exception(prefaceAndMessage);
}

void CommonDDLDef::ValidateName(const DDLString& name) const
{
    if (name.empty())
        NameError("field cannot be empty");

    if (!std::isalpha(name[0]))
        NameError(std::format("field cannot begin with \"{}\"", name[0]));

    for (auto i = 1u; i < name.length(); i++)
    {
        if (!std::isalnum(name[i]) && name.substr(i, 1) != "_")
            NameError(std::format("field cannot contain \"{}\"", name[i]));
    }

    const auto& it = DDL_KEYWORDS.find(name);
    if (it != DDL_KEYWORDS.end())
        NameError(std::format("field cannot be reserved keyword \"{}\"", it->data()));
}
