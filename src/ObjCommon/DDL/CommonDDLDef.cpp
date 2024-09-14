
/*
CommonDDLDef::CommonDDLDef(const int version, const std::string& filename)
    : m_version(version),
      m_size(0u),
      m_filename(std::move(filename))
{
}

CommonDDLInclude::CommonDDLInclude(const int version, const std::string& filename)
    : CommonDDLDef::CommonDDLDef(version, filename)
{

}

std::optional<CommonDDLStructDef&> CommonDDLDef::GetStructByName(const DDLString& name, bool checkIncludes)
{
    if (m_structs.find(name) != m_structs.end())
        return m_structs[name];

    if (checkIncludes)
    {
        for (auto& include : GetIncludes())
        {
            if (include.m_structs.find(name) != include.m_structs.end())
                return include.m_structs[name];
        }
    }
    return std::nullopt;
}

std::optional<CommonDDLEnumDef&> CommonDDLDef::GetEnumByName(const DDLString& name, bool checkIncludes)
{
    if (m_enums.find(name) != m_enums.end())
        return m_enums[name];

    if (checkIncludes)
    {
        for (auto& include : GetIncludes())
        {
            if (include.m_enums.find(name) != include.m_enums.end())
                return include.m_enums[name];
        }
    }
    return std::nullopt;
}

std::optional<CommonDDLStructDef&> CommonDDLDef::GetStructByIndex(const size_t index)
{
    if (index < m_structs.size())
    {
    }

    return std::nullopt;
}

std::optional<CommonDDLEnumDef&> CommonDDLDef::GetEnumByIndex(const int index)
{
    if (index >= 0 && index < m_enums.size())
    {
    }

    return std::nullopt;
}

std::vector<CommonDDLInclude>& CommonDDLDef::GetIncludes()
{
    return m_includes;
}

void CommonDDLDef::SetIncludes(std::vector<CommonDDLInclude>& includes)
{
    m_includes = includes;
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

const size_t CommonDDLDef::TypeToStructIndex(const DDLString& typeName) const noexcept
{
    auto i = 0u;
    for (const auto& [k, struc] : m_structs)
    {
        if (k == typeName)
            return i;
        i++;
    }

    return 0u;
}

const int CommonDDLDef::TypeToEnumIndex(const DDLString& typeName) const noexcept
{
    auto i = 0u;
    for (const auto& [k, enum_] : m_enums)
    {
        if (k == typeName)
            return i;
        i++;
    }

    return -1;
}

const std::optional<DDLString> CommonDDLDef::StructIndexToType(const size_t index) const noexcept
{
    auto i = 0u;
    for (const auto& [k, struc] : m_structs)
    {
        if (i == index)
            return struc.m_name;
        i++;
    }

    return std::nullopt;
}

const std::optional<DDLString> CommonDDLDef::EnumIndexToType(const int index) const noexcept
{
    auto i = 0u;
    for (const auto& [k, enum_] : this->m_enums)
    {
        if (i == index)
        {
            return enum_.m_name;
        }
        i++;
    }

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
    for (auto& [k, struc] : m_structs)
    {
        struc.ResetCalculated();
        for (auto& [k2, member] : struc.m_members)
            member.ResetCalculated();
    }
    for (auto& [k, enum_] : m_enums)
        enum_.ResetCalculated();

    m_in_calculation.clear();
    m_in_calculation.resize(m_structs.size());
}

bool CommonDDLDef::Calculate()
{
    PreCalculate();

    try
    {
        for (auto& [k, struc] : m_structs)
        {
            if (k == "root")
            {
                struc.Calculate();
                break;
            }
        }

        for (auto& [k, struc] : m_structs)
            // Structs can canonically be linked into the asset, but do not change the size of the ddl buffer as they are not in the root struct.
            // Treyarch likely had an include system that would paste structs from an external file into the the defs that include.
            // Or they had an export/import system.
            if (struc.GetRefCount())
                m_size += struc.m_size;

        // Link unreferenced structs anyway
        for (auto& [k, struc] : m_structs)
            if (!struc.GetRefCount())
                struc.Calculate();

        // Link unreferenced enums anyway
        for (auto& [k, enum_] : m_enums)
            if (!enum_.GetRefCount())
                enum_.CalculateHashes();

        for (auto& [k, enum_] : m_enums)
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
    for (const auto& [k, struc] : m_structs)
    {
        if (k == "root")
        {
            struc.Validate();
            break;
        }

        LogicError("root struct entry point missing");
    }

    // Check reference counts for enums, and structs to determine unused enums/structs.
    // If a struct is unused it will still be linked into the asset, but it won't contribute to the size of the def.
    // Track, and validate these unused enums/structs so that the user can be alerted to a mistake(they wanted to use the struct they made, but forgot to
    // actually reference it in root). Treyarch canonically allowed defs to include structs/enums from common files, the evidence is that there are multiple
    // instances of the same struct appearing in multiple defs, with instances of it not having any references occuring multiple times.

    for (const auto& [k, enum_] : m_enums)
    {
        enum_.ReferenceCount();
        if (!enum_.GetRefCount())
            enum_.Validate();
    }

    for (const auto& [k, struc] : m_structs)
    {
        struc.ReferenceCount();
        if (!struc.GetRefCount())
            struc.Validate();
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

void CommonDDLDef::AddStructFromInclude(CommonDDLStructDef& includeStruct)
{
    if (m_structs.find(includeStruct.m_name) == m_structs.end())
    {
        m_structs.emplace(includeStruct.m_name, includeStruct);
        includeStruct.Resolve();
    }
}

void CommonDDLDef::AddEnumFromInclude(CommonDDLEnumDef& includeEnum)
{
    if (m_enums.find(includeEnum.m_name) == m_enums.end())
    {
        m_enums.emplace(includeEnum.m_name, includeEnum);
    }
}

void CommonDDLDef::ResolveCustomTypes()
{
    std::vector<CommonDDLStructDef> includedStructs;
    std::vector<CommonDDLEnumDef> includedEnums;

    // Determine what includes are referenced
    for (auto& [k, struc] : m_structs)
    {
        for (auto& [k, member] : struc.m_members)
        {

            if (!member.data.m_link_data.m_external_index)
            {
                member.data.m_link_data.m_type_enum = member.data.NameToType();
                if (member.data.m_link_data.m_type_enum == DDL_TYPE_COUNT)
                    LogicError("<UNIMPLEMENTED>");
            }
            else
            {
                member.data.m_link_data.m_struct.emplace(m_structs[member.data.m_link_data.m_external_index].name);
                member.type = member.data.m_link_data.m_struct.value();
                size_t flags = member.data.m_link_data.m_user_type_flags | DDL_USER_TYPE_STRUCT;
                member.data.m_link_data.m_user_type_flags = static_cast<ddlUserDefinedTypeFlags_e>(flags);
            }

            member.data.m_link_data.enumIndex = -1;

            if (!member.enum_.has_value())
                continue;

            member.data.m_link_data.enumIndex = member.TypeToEnumIndex();
            if (member.data.m_link_data.enumIndex > -1)
            {
                member.enum_.emplace(jDDLDef.enums[member.data.m_link_data.enumIndex].name);
                size_t flags = member.data.m_link_data.m_user_type_flags | DDL_USER_TYPE_ENUM;
                member.data.m_link_data.m_user_type_flags = static_cast<ddlUserDefinedTypeFlags_e>(flags);
            }
            else
                LogicError("<UNIMPLEMENTED>");
        }
    }

    m_structs.insert(m_structs.end(), includedStructs.begin(), includedStructs.end());
    m_enums.insert(m_enums.end(), includedEnums.begin(), includedEnums.end());

    for (auto& struc : m_structs)
    {
        for (auto& member : struc.m_members)
        {
            member.m_link_data.m_type_enum = member.NameToType();
            if (member.IsValidType())
            {
            }
        }
    }
}
*/