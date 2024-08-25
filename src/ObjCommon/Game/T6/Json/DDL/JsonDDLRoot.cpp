#include "JsonDDLRoot.h"

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
        std::string prefaceAndMessage = std::format("DDL Member: {} Type: {} Struct {}: ", name, type, parentStruct.value_or("unknown")) + message;
        throw JsonDDLMemberDef::JsonDDLMemberDefException(prefaceAndMessage);
    }

    bool JsonDDLMemberDef::IsStandardSize(const size_t size, const T6::ddlPrimitiveTypes_e type, const size_t arraySize) const
    {
        const auto memberSize = (size / arraySize);
        if (type < T6::ddlPrimitiveTypes_e::DDL_BYTE_TYPE || type > T6::ddlPrimitiveTypes_e::DDL_UINT64_TYPE)
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

    ddlPermissionTypes_e T6::JsonDDLMemberDef::NameToPermissionType(const std::string& typeName) const
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

    void T6::JsonDDLMemberDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        JsonDDLMemberDef::Name(jDDLDef);
        JsonDDLMemberDef::Type(jDDLDef);
        JsonDDLMemberDef::Permission(jDDLDef);
        JsonDDLMemberDef::Array(jDDLDef);
        JsonDDLMemberDef::MaxCharacters(jDDLDef);
        JsonDDLMemberDef::Enum_(jDDLDef);
        JsonDDLMemberDef::BitFields();
        JsonDDLMemberDef::Range();
        JsonDDLMemberDef::FixedPoint();
    }

    size_t JsonDDLMemberDef::CalculateBitSize(T6::JsonDDLDef& jDDLDef)
    {
        if (memberSize.has_value())
            return memberSize.value();

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
                auto& structDef = jDDLDef.structs.at(structIndex.value());
                size = structDef.CalculateBitSize(jDDLDef);
            }
        }

        if (!size)
            MemberLogicError("could not calculate size");

        memberSize.emplace(size * arrayCount.value_or(1));
        return memberSize.value();
    }

    void JsonDDLMemberDef::StoreParent(JsonDDLDef& jDDLDef)
    {
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            for (auto j = 0u; j < jDDLDef.structs[i].members.size(); j++)
            {
                jDDLDef.structs[i].members[j].parentStruct.emplace(jDDLDef.structs[i].name);
            }
        }
    }

    void JsonDDLMemberDef::Name(const JsonDDLDef& jDDLDef) const
    {
        if (name.empty())
            MemberLogicError("Value cannot be negative");

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
                MemberLogicError("Value cannot be negative");
        }
    }

    void JsonDDLMemberDef::Type(const JsonDDLDef& jDDLDef) const
    {
        const auto typeEnum = JsonDDLMemberDef::NameToType(type);
        if (typeEnum <= DDL_STRING_TYPE)
            return;
        if (typeEnum < DDL_TYPE_COUNT)
            MemberLogicError("Value cannot be negative");

        std::string lowerName1(name);
        utils::MakeStringLowerCase(lowerName1);
        std::string lowerName2;
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            lowerName2.assign(jDDLDef.structs[i].name);
            if (lowerName1 != lowerName2)
                continue;

            return;
        }

        MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::Permission(const JsonDDLDef& jDDLDef) const
    {
        if (permission.empty())
            return;

        const auto typeEnum = NameToPermissionType(permission);
        if (typeEnum <= DDL_PERM_UNSPECIFIED || typeEnum >= DDL_PERM_COUNT)
            MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::Array(const JsonDDLDef& jDDLDef) const
    {
        if (!arrayCount.has_value())
            return;
        if (enum_.has_value())
            MemberLogicError("Value cannot be negative");
        if (arrayCount.value() <= 0)
            MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::MaxCharacters(const JsonDDLDef& jDDLDef) const
    {
        const auto isStringType = NameToType(type) == DDL_STRING_TYPE;
        if (isStringType)
        {
            if (!maxCharacters.has_value())
                MemberLogicError("Value cannot be negative");
            if (maxCharacters.value() <= 0)
                MemberLogicError("Value cannot be negative");

            return;
        }
        else if (!maxCharacters.has_value())
            return;

        MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::Enum_(const JsonDDLDef& jDDLDef) const
    {
        if (!enum_.has_value())
            return;
        if (arrayCount.has_value())
            MemberLogicError("Value cannot be negative");

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

        MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::BitFields() const
    {
        if (!limits.has_value())
            return;

        if (!limits->bits.has_value())
            return;

        const auto typeEnum = NameToType(type);
        if ((DDL_TYPE_FEATURES[typeEnum].flags & DDL_FLAG_BITFIELDS) == 0)
            MemberLogicError("Value cannot be negative");
        if (limits->range.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
            MemberLogicError("Value cannot be negative");
        if (limits->bits.value() > DDL_TYPE_FEATURES[typeEnum].size)
            MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::Range() const
    {
        if (!limits.has_value())
            return;

        if (!limits->range.has_value())
            return;

        const auto typeEnum = NameToType(type);
        if ((DDL_TYPE_FEATURES[typeEnum].flags & DDL_FLAG_LIMITS) == 0)
            MemberLogicError("Value cannot be negative");
        if (limits->bits.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
            MemberLogicError("Value cannot be negative");
        if (limits->range.value() > DDL_TYPE_FEATURES[typeEnum].max)
            MemberLogicError("Value cannot be negative");
    }

    void JsonDDLMemberDef::FixedPoint() const
    {
        if (!limits.has_value())
            return;

        if (!limits->fixedMagnitudeBits.has_value() || !limits->fixedPrecisionBits.has_value())
            return;

        const auto typeEnum = NameToType(type);
        if (typeEnum != DDL_FIXEDPOINT_TYPE)
            MemberLogicError("Value cannot be negative");
        if (limits->range.has_value() || limits->bits.has_value())
            MemberLogicError("Value cannot be negative");
        if ((limits->fixedMagnitudeBits.value() + limits->fixedPrecisionBits.value()) > DDL_TYPE_FEATURES[typeEnum].size)
            MemberLogicError("Value cannot be negative");
    }

    void JsonDDLStructDef::StructLogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef.value_or("unknown")) + message;
        throw JsonDDLStructDefException(prefaceAndMessage);
    }

    void JsonDDLStructDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        Name(jDDLDef);
    }

    size_t JsonDDLStructDef::CalculateBitSize(JsonDDLDef& jDDLDef)
    {
        if (structSize.has_value())
            return structSize.value();

        auto size = 0u;
        for (auto i = 0u; i < members.size(); i++)
        {
            size += members[i].CalculateBitSize(jDDLDef);
        }

        structSize.emplace(size);
        return size;
    }

    JsonDDLStructDef::JsonDDLStructDefException::JsonDDLStructDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLStructDef::Name(const JsonDDLDef& jDDLDef) const
    {
        if (name.empty())
            JsonDDLStructDef::StructLogicError("Value cannot be negative");

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
                JsonDDLStructDef::StructLogicError("Value cannot be negative");
        }

        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            other.assign(jDDLDef.enums[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                JsonDDLStructDef::StructLogicError("Value cannot be negative");
        }
    }

    void JsonDDLStructDef::Members(const JsonDDLDef& jDDLDef) const
    {
        if (members.empty())
            JsonDDLStructDef::StructLogicError("Value cannot be negative");
    }

    void JsonDDLEnumDef::EnumLogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef.value_or("unknown")) + message;
        throw JsonDDLEnumDef::JsonDDLEnumDefException(prefaceAndMessage);
    }

    void JsonDDLEnumDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        JsonDDLEnumDef::Members(jDDLDef);
    }

    JsonDDLEnumDef::JsonDDLEnumDefException::JsonDDLEnumDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLEnumDef::Name(const JsonDDLDef& jDDLDef) const
    {
        return;
    }

    void JsonDDLEnumDef::Members(const JsonDDLDef& jDDLDef) const
    {
        if (members.empty())
            JsonDDLEnumDef::EnumLogicError("Value cannot be negative");
    }

    void JsonDDLDef::DefLogicError(const std::string& message) const
    {
        assert(filename.has_value());

        std::string prefaceAndMessage = std::format("DDL Def: {} ", filename) + message;
        throw JsonDDLDef::JsonDDLDefException(prefaceAndMessage);
    }

    void JsonDDLDef::Validate() const
    {
        JsonDDLDef::Root();
    }

    size_t JsonDDLDef::CalculateBitSize(JsonDDLDef& jDDLDef)
    {
        for (auto struc : jDDLDef.structs)
        {
            std::string lowerName(struc.name);
            utils::MakeStringLowerCase(lowerName);
            if (lowerName == "root")
            {
                defSize.emplace(structs[0].CalculateBitSize(jDDLDef));
                return defSize.value();
            }
        }

        JsonDDLDef::DefLogicError("root struct entry point missing");

        return 0;
    }

    JsonDDLDef::JsonDDLDefException::JsonDDLDefException(std::string& message)
        : std::runtime_error(message)
    {
    }

    void JsonDDLDef::Root() const
    {
        std::string lowerName;
        for (auto i = 0u; i < structs.size(); i++)
        {
            lowerName.assign(structs[i].name);
            utils::MakeStringLowerCase(lowerName);

            if (lowerName == "root")
                return;
        }

        JsonDDLDef::DefLogicError("Value cannot be negative");
    }
} // namespace T6
