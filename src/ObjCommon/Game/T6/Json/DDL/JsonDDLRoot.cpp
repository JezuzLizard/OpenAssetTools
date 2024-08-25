#include "JsonDDLRoot.h"

#include "Game/T6/CommonT6.h"

#include <iostream>

#include "Utils/StringUtils.h"

namespace T6
{
    inline const std::string DDL_TYPE_NAMES[]{
        "byte",
        "short",
        "uint",
        "int",
        "uint64",
        "float",
        "fixed_float",
        "string",
        "struct",
        "enum",
        "",
    };

    inline const std::string DDL_PERM_NAMES[]{
        "unspecified",
        "client",
        "server",
        "both",
        "",
    };

    inline const DDLFeatureSupport DDL_TYPE_FEATURES[] = {
        {
         .size = sizeof(char) * CHAR_BIT,
         .flags = 0,
         .max = UCHAR_MAX,
         .min = 0,
         },
        {
         .size = sizeof(short) * CHAR_BIT,
         .flags = 0,
         .max = USHRT_MAX,
         .min = 0,
         },
        {
         .size = sizeof(int) * CHAR_BIT,
         .flags = DDL_FLAG_LIMITS | DDL_FLAG_BITFIELDS,
         .max = UINT_MAX,
         .min = 0,
         },

        {
         .size = sizeof(int) * CHAR_BIT,
         .flags = DDL_FLAG_SIGNED | DDL_FLAG_LIMITS | DDL_FLAG_BITFIELDS,
         .max = INT_MAX,
         .min = INT_MIN,
         },

        {
         .size = sizeof(float) * CHAR_BIT,
         .flags = DDL_FLAG_SIGNED,
         .max = INT_MAX,
         .min = INT_MIN,
         },
        {
         .size = sizeof(uint64_t) * CHAR_BIT,
         .flags = 0,
         .max = UINT64_MAX,
         .min = 0,
         },

        {
         .size = sizeof(float) * CHAR_BIT,
         .flags = 0,
         .max = 0,
         .min = 0,
         },
    };

    JsonDDLMemberDef::JsonDDLMemberDefException::JsonDDLMemberDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLMemberDef::MemberLogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Member: {} Type: {} Struct {}: ", name, type, parentStruct) + message;
        throw JsonDDLMemberDefException(prefaceAndMessage);
    }

    bool JsonDDLMemberDef::IsStandardSize(const size_t size, const T6::ddlPrimitiveTypes_e type, const size_t arraySize) const
    {
        const auto memberSize = (size / arraySize);
        if (type < DDL_BYTE_TYPE || type > DDL_UINT64_TYPE)
            return false;
        return DDL_TYPE_FEATURES[type].size == memberSize;
    }

    std::string JsonDDLMemberDef::PermissionTypeToName(const ddlPermissionTypes_e type) const
    {
        if (type <= DDL_PERM_UNSPECIFIED || type >= DDL_PERM_COUNT)
            return static_cast<int>(type) + "_unknown";

        return DDL_PERM_NAMES[type];
    }

    std::string JsonDDLMemberDef::TypeToName(const ddlPrimitiveTypes_e type) const
    {
        if (type >= DDL_TYPE_COUNT)
            return static_cast<int>(type) + "_unknown";

        return DDL_TYPE_NAMES[type];
    }

    ddlPrimitiveTypes_e JsonDDLMemberDef::NameToType(const std::string& typeName) const
    {
        std::string copy(typeName);
        utils::MakeStringLowerCase(copy);

        for (auto i = 0; i < DDL_TYPE_COUNT; i++)
        {
            if (copy == DDL_TYPE_NAMES[i])
                return static_cast<ddlPrimitiveTypes_e>(i);
        }

        return DDL_TYPE_COUNT;
    }

    ddlPermissionTypes_e JsonDDLMemberDef::NameToPermissionType(const std::string& typeName) const
    {
        std::string copy(typeName);
        utils::MakeStringLowerCase(copy);

        for (auto i = 1; i < DDL_PERM_COUNT; i++)
        {
            if (copy == DDL_PERM_NAMES[i])
                return static_cast<ddlPermissionTypes_e>(i);
        }

        return DDL_PERM_UNSPECIFIED;
    }

    void JsonDDLMemberDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        referenceCount++;
        if (calculated)
            return;

        if (enum_.has_value())
        {
            auto& enumDef = jDDLDef.enums.at(enumIndex.value());
            enumDef.Validate(jDDLDef);
        }

        auto size = 0u;
        const auto typeEnum = NameToType(type);
        if (typeEnum <= DDL_FIXEDPOINT_TYPE)
        {
            if (!limits.has_value())
                size = DDL_TYPE_FEATURES[typeEnum].size;
            else if (limits.value().bits.has_value())
                size = limits.value().bits.value();
            else if (limits.value().range.has_value())
                size = std::bit_width(limits.value().range.value());
            else if (limits.value().fixedMagnitudeBits.has_value() && limits.value().fixedPrecisionBits.has_value())
                size = limits.value().fixedMagnitudeBits.value() + limits.value().fixedPrecisionBits.value();
        }
        else if (typeEnum == DDL_STRING_TYPE)
        {
            if (maxCharacters.has_value())
                size = maxCharacters.value() * CHAR_BIT;
        }
        else if (typeEnum == DDL_STRUCT_TYPE)
        {
            if (structIndex.has_value())
            {
                if (jDDLDef.inCalculation.contains(structIndex.value()))
                    MemberLogicError("circular dependency detected");

                auto& structDef = jDDLDef.structs.at(structIndex.value());
                jDDLDef.inCalculation.insert(structIndex.value());
                structDef.Calculate(jDDLDef);
                size = structDef.structSize.value();
            }
        }

        if (!size)
            MemberLogicError("could not calculate size");

        calculated = true;
        jDDLDef.inCalculation.clear();

        memberSize.emplace(size * arrayCount.value_or(1));


        Name(jDDLDef);
        Type(jDDLDef);
        Permission(jDDLDef);
        Array(jDDLDef);
        MaxCharacters(jDDLDef);
        Enum_(jDDLDef);
        BitFields();
        Range();
        FixedPoint();
    }

    void JsonDDLMemberDef::Calculate(T6::JsonDDLDef& jDDLDef)
    {
        referenceCount++;
        if (calculated)
            return;

        if (enum_.has_value())
        {
            auto& enumDef = jDDLDef.enums.at(enumIndex.value());
            enumDef.CalculateHashes();
        }

        auto size = 0u;
        const auto typeEnum = NameToType(type);
        if (typeEnum <= DDL_FIXEDPOINT_TYPE)
        {
            if (!limits.has_value())
                size = DDL_TYPE_FEATURES[typeEnum].size;
            else if (limits.value().bits.has_value())
                size = limits.value().bits.value();
            else if (limits.value().range.has_value())
                size = std::bit_width(limits.value().range.value());
            else if (limits.value().fixedMagnitudeBits.has_value() && limits.value().fixedPrecisionBits.has_value())
                size = limits.value().fixedMagnitudeBits.value() + limits.value().fixedPrecisionBits.value();
        }
        else if (typeEnum == DDL_STRING_TYPE)
        {
            if (maxCharacters.has_value())
                size = maxCharacters.value() * CHAR_BIT;
        }
        else if (typeEnum == DDL_STRUCT_TYPE)
        {
            if (structIndex.has_value())
            {
                if (jDDLDef.inCalculation.contains(structIndex.value()))
                    MemberLogicError("circular dependency detected");

                auto& structDef = jDDLDef.structs.at(structIndex.value());
                jDDLDef.inCalculation.insert(structIndex.value());
                structDef.Calculate(jDDLDef);
                size = structDef.structSize.value();
            }
        }

        if (!size)
            MemberLogicError("could not calculate size");

        calculated = true;
        jDDLDef.inCalculation.clear();

        memberSize.emplace(size * arrayCount.value_or(1));
    }

    void JsonDDLMemberDef::StoreParent(JsonDDLDef& jDDLDef)
    {
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            for (auto j = 0u; j < jDDLDef.structs[i].members.size(); j++)
            {
                jDDLDef.structs[i].members[j].parentStruct.assign(jDDLDef.structs[i].name);
            }
        }
    }

    void JsonDDLMemberDef::Name(const JsonDDLDef& jDDLDef) const
    {
        if (name.empty())
            MemberLogicError("name field cannot be empty");

        std::string primary(name);
        utils::MakeStringLowerCase(primary);
        std::string other;

        size_t nameRedefinitions = 0;
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            other.assign(jDDLDef.structs[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                MemberLogicError("member cannot have the same name as a struct");
        }

        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            other.assign(jDDLDef.enums[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                MemberLogicError("member cannot have the same name as an enum");
        }
    }

    void JsonDDLMemberDef::Type(const JsonDDLDef& jDDLDef) const
    {
        const auto typeEnum = JsonDDLMemberDef::NameToType(type);
        if (typeEnum <= DDL_STRING_TYPE)
            return;
        if (typeEnum < DDL_TYPE_COUNT)
            MemberLogicError("type cannot directly be defined as enum or struct");

        std::string lowerName1(name);
        utils::MakeStringLowerCase(lowerName1);
        std::string lowerName2;
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            lowerName2.assign(jDDLDef.structs[i].name);
            if (lowerName1 == lowerName2)
                return;
        }

        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            lowerName2.assign(jDDLDef.enums[i].name);
            if (lowerName1 == lowerName2)
                MemberLogicError("type field cannot be set to an enum, the enum_ field is required instead");
        }

        MemberLogicError("no definition found for type value");
    }

    void JsonDDLMemberDef::Permission(const JsonDDLDef& jDDLDef) const
    {
        if (permission.empty())
            return;

        const auto typeEnum = NameToPermissionType(permission);
        if (typeEnum <= DDL_PERM_UNSPECIFIED || typeEnum >= DDL_PERM_COUNT)
            MemberLogicError("permission must be client, server, or both");
    }

    void JsonDDLMemberDef::Array(const JsonDDLDef& jDDLDef) const
    {
        if (!arrayCount.has_value())
            return;
        if (enum_.has_value())
            MemberLogicError("arrayCount field cannot be combined with enum_ field");
        if (arrayCount.value() <= 0)
            MemberLogicError("arrayCount cannot be 0");
    }

    void JsonDDLMemberDef::MaxCharacters(const JsonDDLDef& jDDLDef) const
    {
        const auto isStringType = NameToType(type) == DDL_STRING_TYPE;
        if (isStringType)
        {
            if (!maxCharacters.has_value())
                MemberLogicError("string type requires maxCharacters field");
            if (maxCharacters.value() <= 0)
                MemberLogicError("maxCharacters cannot be 0");

            return;
        }
        else if (!maxCharacters.has_value())
            return;

        MemberLogicError("maxCharacters field cannot be used with types other than string");
    }

    void JsonDDLMemberDef::Enum_(const JsonDDLDef& jDDLDef) const
    {
        if (!enum_.has_value())
            return;
        if (arrayCount.has_value())
            MemberLogicError("arrayCount field cannot be combined with enum_ field");

        std::string lowerName1(name);
        utils::MakeStringLowerCase(lowerName1);
        std::string lowerName2;
        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            lowerName2.assign(jDDLDef.enums[i].name);
            if (lowerName1 != lowerName2)
                continue;

            return;
        }

        MemberLogicError("no definition for enum");
    }

    void JsonDDLMemberDef::BitFields() const
    {
        if (!limits.has_value())
            return;

        if (!limits->bits.has_value())
            return;

        const auto typeEnum = NameToType(type);
        if ((DDL_TYPE_FEATURES[typeEnum].flags & DDL_FLAG_BITFIELDS) == 0)
            MemberLogicError("type does not support bits field");
        if (limits->range.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
            MemberLogicError("range, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with bits field");
        if (limits->bits.value() > DDL_TYPE_FEATURES[typeEnum].size)
            MemberLogicError("bits exceeds maximum possible bits for type");
    }

    void JsonDDLMemberDef::Range() const
    {
        if (!limits.has_value())
            return;

        if (!limits->range.has_value())
            return;

        const auto typeEnum = NameToType(type);
        if ((DDL_TYPE_FEATURES[typeEnum].flags & DDL_FLAG_LIMITS) == 0)
            MemberLogicError("does not support range field");
        if (limits->bits.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
            MemberLogicError("bits, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with range field");
        if (limits->range.value() > DDL_TYPE_FEATURES[typeEnum].max)
            MemberLogicError("range exceeds maximum possible value for type");
    }

    void JsonDDLMemberDef::FixedPoint() const
    {
        if (!limits.has_value())
            return;

        const auto typeEnum = NameToType(type);
        if (!limits->fixedMagnitudeBits.has_value() && !limits->fixedPrecisionBits.has_value())
            if (typeEnum == DDL_FIXEDPOINT_TYPE)
                MemberLogicError("fixed_float requires both fixedMagnitudeBits, and fixedPrecisionBits fields");
            return;

        if (typeEnum != DDL_FIXEDPOINT_TYPE)
            MemberLogicError("type must be fixed_float in order to use fixedMagnitudeBits, and FixedPrecisionBits");
        if (limits->range.has_value() || limits->bits.has_value())
            MemberLogicError("range, and bits fields cannot be used with fixed_float type");
        if ((limits->fixedMagnitudeBits.value() + limits->fixedPrecisionBits.value()) > DDL_TYPE_FEATURES[typeEnum].size)
            MemberLogicError("magnitude, and precision bits combined cannot exceed 32 bits");
    }

    void JsonDDLStructDef::StructLogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef) + message;
        throw JsonDDLStructDefException(prefaceAndMessage);
    }

    void JsonDDLStructDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        Name(jDDLDef);
        Members(jDDLDef);
    }

    void JsonDDLStructDef::Calculate(JsonDDLDef& jDDLDef)
    {
        referenceCount++;
        if (calculated)
            return;

        auto size = 0u;
        for (auto i = 0u; i < members.size(); i++)
        {
            members[i].Calculate(jDDLDef);
            size += members[i].memberSize.value();
        }

        structSize.emplace(size);

        calculated = true;
    }

    JsonDDLStructDef::JsonDDLStructDefException::JsonDDLStructDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLStructDef::Name(const JsonDDLDef& jDDLDef) const
    {
        if (name.empty())
            StructLogicError("name cannot be blank");

        std::string primary(name);
        utils::MakeStringLowerCase(primary);
        std::string other;

        size_t nameRedefinitions = 0;
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            other.assign(jDDLDef.structs[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                StructLogicError("multiple redefinitions of struct");
        }

        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            other.assign(jDDLDef.enums[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                StructLogicError("struct cannot have same name as enum");
        }
    }

    void JsonDDLStructDef::Members(const JsonDDLDef& jDDLDef) const
    {
        if (members.empty())
            StructLogicError("struct must have at least one member");

        for (const auto& member : members)
            member.Validate(jDDLDef);
    }

    void JsonDDLStructDef::ReferenceCount(const JsonDDLDef& jDDLDef) const
    {
        if (!referenceCount)
            StructLogicError("an unreferenced struct cannot be linked");
    }

    void JsonDDLEnumDef::EnumLogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef.value_or("unknown")) + message;
        throw JsonDDLEnumDefException(prefaceAndMessage);
    }

    void JsonDDLEnumDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        Members(jDDLDef);
    }

    void JsonDDLEnumDef::CalculateHashes()
    {
        referenceCount++;
        if (calculated)
            return;

        for (auto i = 0; i < members.size(); i++)
        {
            int hash = Common::Com_HashString(members[i].c_str());
            hashTable.insert(hash, i);
        }

        calculated = true;
    }

    JsonDDLEnumDef::JsonDDLEnumDefException::JsonDDLEnumDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLEnumDef::Name(const JsonDDLDef& jDDLDef) const
    {
        std::set<std::string> occurrences;

        auto i = 0u;
        for (; i < members.size(); i++)
        {
            if (!occurrences.contains(members[i]))
            {
                occurrences.insert(members[i]);
                continue;
            }

            EnumLogicError(std::format("multiple occurrences of enum member \"{}\"", members[i]));
        }
    }

    void JsonDDLEnumDef::Members(const JsonDDLDef& jDDLDef) const
    {
        if (members.empty())
            EnumLogicError("enum must have at least one member");
    }

    void JsonDDLEnumDef::ReferenceCount(const JsonDDLDef& jDDLDef) const
    {
        if (!referenceCount)
            EnumLogicError("an unreferenced enum cannot be linked");
    }

    void JsonDDLDef::DefLogicError(const std::string& message) const
    {
        assert(filename.has_value());

        std::string prefaceAndMessage = std::format("DDL Def: {} ", filename) + message;
        throw JsonDDLDefException(prefaceAndMessage);
    }

    std::optional<size_t> JsonDDLDef::TypeToStructIndex(const JsonDDLDef& jDDLDef, const std::string& typeName) const
    {
        std::string lowerType(typeName);
        utils::MakeStringLowerCase(lowerType);
        for (auto i = 0u; i < structs.size(); i++)
        {
            std::string lowerName(structs[i].name);
            utils::MakeStringLowerCase(lowerName);
            if (lowerName == lowerType)
                return i;
        }

        return std::nullopt;
    }

    std::optional<size_t> JsonDDLDef::TypeToEnumIndex(const JsonDDLDef& jDDLDef, const std::string& typeName) const
    {
        std::string lowerType(typeName);
        utils::MakeStringLowerCase(lowerType);
        for (auto i = 0u; i < enums.size(); i++)
        {
            std::string lowerName(enums[i].name);
            utils::MakeStringLowerCase(lowerName);
            if (lowerName == lowerType)
                return i;
        }

        return std::nullopt;
    }

    std::optional<std::string> JsonDDLDef::StructIndexToType(const JsonDDLDef& jDDLDef, const size_t index) const
    {
        if (structs.size() < index)
            return structs[index].name;

        return std::nullopt;
    }

    std::optional<std::string> JsonDDLDef::EnumIndexToType(const JsonDDLDef& jDDLDef, const size_t index) const
    {
        if (enums.size() < index)
            return enums[index].name;

        return std::nullopt;
    }

    void JsonDDLDef::Validate() const
    {
        Root();
    }

    void JsonDDLDef::Calculate()
    {
        for (auto i = 0; i < structs.size(); i++)
        {
            std::string lowerName(structs[i].name);
            utils::MakeStringLowerCase(lowerName);
            if (lowerName == "root")
            {
                structs[i].Calculate(*this);
            }
        }

        DefLogicError("root struct entry point missing");
    }

    JsonDDLDef::JsonDDLDefException::JsonDDLDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLDef::Root() const
    {
        std::string lowerName;
        auto i = 0u;
        for ( ; i < structs.size(); i++)
        {
            lowerName.assign(structs[i].name);
            utils::MakeStringLowerCase(lowerName);

            if (lowerName != "root")
                continue;

            DefLogicError("root struct entry point missing");
        }

        structs[i].Validate(*this);
    }
} // namespace T6
