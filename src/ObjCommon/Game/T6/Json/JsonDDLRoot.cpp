#include "JsonDDLRoot.h"

#include <iostream>

#include "Utils/StringUtils.h"

namespace T6
{
    class DDLFeatureSupport
    {
    public:
        size_t size;
        size_t flags;
        uint64_t max;
        int64_t min;
    };

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

    class JsonDDLMemberLimits
    {
    public:
        std::optional<size_t> bits;
        std::optional<size_t> range;
        std::optional<size_t> fixedPrecisionBits;
        std::optional<size_t> fixedMagnitudeBits;
    };

    class JsonDDLMemberDef
    {
    public:
        std::string name;
        std::string type;
        std::optional<std::string> enum_;
        std::optional<size_t> enumIndex;
        std::string permission;
        std::optional<size_t> maxCharacters;
        std::optional<size_t> structIndex;
        std::optional<size_t> arrayCount;
        std::optional<size_t> memberSize;
        std::optional<std::string> parentStruct;
        std::optional<size_t> offset;
        std::optional<JsonDDLMemberLimits> limits;
        

        class JsonDDLMemberDefException : public std::runtime_error
        {
        public:
            JsonDDLMemberDefException(std::string& message)
                : std::runtime_error(message)
            {
            }
        };

        void MemberLogicError(const std::string& message) const
        {
            std::string prefaceAndMessage = std::format("DDL Member: {} Type: {} Struct {}: ", name, type, parentStruct.value_or("unknown")) + message;
            throw JsonDDLMemberDefException(prefaceAndMessage);
        }

        void Validate(const JsonDDLDef& jDDLDef) const
        {
            StoreParent(jDDLDef);
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

        size_t CalculateBitSize(JsonDDLDef& jDDLDef)
        {
            if (memberSize.has_value())
                return memberSize.value();

            auto size = 0u;
            const auto typeEnum = DDL::NameToType(type);
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
                    auto structDef = jDDLDef.structs.at(structIndex.value());
                    size = structDef.CalculateBitSize(jDDLDef);
                }
            }

            if (!size)
                MemberLogicError("could not calculate size");

            memberSize.emplace(size * arrayCount.value_or(1));
            return memberSize.value();
        }

    private:

        void StoreParent(JsonDDLDef& jDDLDef)
        {
            for (auto i = 0u; i < jDDLDef.structs.size(); i++)
            {
                for (auto j = 0u; j < jDDLDef.structs[i].members.size(); j++)
                {
                    jDDLDef.structs[i].members[j].parentStruct.emplace(jDDLDef.structs[i].name);
                }
            }
        }

        void Name(const JsonDDLDef& jDDLDef) const
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

        void Type(const JsonDDLDef& jDDLDef) const
        {
            const auto typeEnum = DDL::NameToType(type);
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

        void Permission(const JsonDDLDef& jDDLDef) const
        {
            if (permission.empty())
                return;

            const auto typeEnum = DDL::NameToPermissionType(permission);
            if (typeEnum <= DDL_PERM_UNSPECIFIED || typeEnum >= DDL_PERM_COUNT)
                MemberLogicError("Value cannot be negative");
        }

        void Array(const JsonDDLDef& jDDLDef) const
        {
            if (!arrayCount.has_value())
                return;
            if (enum_.has_value())
                MemberLogicError("Value cannot be negative");
            if (arrayCount.value() <= 0)
                MemberLogicError("Value cannot be negative");
        }

        void MaxCharacters(const JsonDDLDef& jDDLDef) const
        {
            const auto isStringType = DDL::NameToType(type) == DDL_STRING_TYPE;
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

        void Enum_(const JsonDDLDef& jDDLDef) const
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

        void BitFields() const
        {
            if (!limits.has_value())
                return;

            if (!limits->bits.has_value())
                return;

            const auto typeEnum = DDL::NameToType(type);
            if ((DDL_TYPE_FEATURES[typeEnum].flags & DDL_FLAG_BITFIELDS) == 0)
                MemberLogicError("Value cannot be negative");
            if (limits->range.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
                MemberLogicError("Value cannot be negative");
            if (limits->bits.value() > DDL_TYPE_FEATURES[typeEnum].size)
                MemberLogicError("Value cannot be negative");
        }

        void Range() const
        {
            if (!limits.has_value())
                return;

            if (!limits->range.has_value())
                return;

            const auto typeEnum = DDL::NameToType(type);
            if ((DDL_TYPE_FEATURES[typeEnum].flags & DDL_FLAG_LIMITS) == 0)
                MemberLogicError("Value cannot be negative");
            if (limits->bits.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
                MemberLogicError("Value cannot be negative");
            if (limits->range.value() > DDL_TYPE_FEATURES[typeEnum].max)
                MemberLogicError("Value cannot be negative");
        }

        void FixedPoint() const
        {
            if (!limits.has_value())
                return;

            if (!limits->fixedMagnitudeBits.has_value() || !limits->fixedPrecisionBits.has_value())
                return;

            const auto typeEnum = DDL::NameToType(type);
            if (typeEnum != DDL_FIXEDPOINT_TYPE)
                MemberLogicError("Value cannot be negative");
            if (limits->range.has_value() || limits->bits.has_value())
                MemberLogicError("Value cannot be negative");
            if ((limits->fixedMagnitudeBits.value() + limits->fixedPrecisionBits.value()) > DDL_TYPE_FEATURES[typeEnum].size)
                MemberLogicError("Value cannot be negative");
        }
    };

    class JsonDDLStructDef
    {
    public:
        std::string name;
        std::optional<size_t> structSize;
        std::vector<JsonDDLMemberDef> members;
        std::optional<std::string> parentDef;

        void StructLogicError(const std::string& message) const
        {
            std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef.value_or("unknown")) + message;
            throw JsonDDLStructDefException(prefaceAndMessage);
        }

        void Validate(const JsonDDLDef& jDDLDef) const
        {
            Name(jDDLDef);
        }

        size_t CalculateBitSize(JsonDDLDef& jDDLDef)
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

    private:
        class JsonDDLStructDefException : public std::runtime_error
        {
        public:
            JsonDDLStructDefException(std::string& message)
                : std::runtime_error(message)
            {
            }
        };

        void Name(const JsonDDLDef& jDDLDef) const
        {
            if (name.empty())
                StructLogicError("Value cannot be negative");

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
                    StructLogicError("Value cannot be negative");
            }

            for (auto i = 0u; i < jDDLDef.enums.size(); i++)
            {
                other.assign(jDDLDef.enums[i].name);
                utils::MakeStringLowerCase(other);
                if (other == primary)
                    nameRedefinitions++;

                if (nameRedefinitions > 1)
                    StructLogicError("Value cannot be negative");
            }
        }

        void Members(const JsonDDLDef& jDDLDef) const
        {
            if (members.empty())
                StructLogicError("Value cannot be negative");
        }
    };

    class JsonDDLEnumDef
    {
    public:
        std::string name;
        std::vector<std::string> members;
        std::optional<std::string> parentDef;

        void EnumLogicError(const std::string& message) const
        {
            std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef.value_or("unknown")) + message;
            throw JsonDDLEnumDefException(prefaceAndMessage);
        }

        void Validate(const JsonDDLDef& jDDLDef) const
        {
            Members(jDDLDef);
        }

    private:
        class JsonDDLEnumDefException : public std::runtime_error
        {
        public:
            JsonDDLEnumDefException(std::string& message)
                : std::runtime_error(message)
            {
            }
        };

        void Name(const JsonDDLDef& jDDLDef) const
        {
            return;
        }

        void Members(const JsonDDLDef& jDDLDef) const
        {
            if (members.empty())
                EnumLogicError("Value cannot be negative");
        }
    };

    class JsonDDLDef
    {
    public:
        std::optional<std::string> filename;
        int version;
        std::optional<size_t> defSize;
        std::vector<JsonDDLEnumDef> enums;
        std::vector<JsonDDLStructDef> structs;

        void DefLogicError(const std::string& message) const
        {
            assert(filename.has_value());

            std::string prefaceAndMessage = std::format("DDL Def: {} ", filename) + message;
            throw JsonDDLDefException(prefaceAndMessage);
        }

        void Validate() const
        {
            Root();
        }

        size_t CalculateBitSize()
        {
            auto size = 0u;
            for (auto i = 0u; i < structs.size(); i++)
            {
                size += structs[i].structSize.value();
            }
        }

    private:

        class JsonDDLDefException : public std::runtime_error
        {
        public:
            JsonDDLDefException(std::string& message)
                : std::runtime_error(message)
            {
            }
        };

        void Root() const
        {
            std::string lowerName;
            for (auto i = 0u; i < structs.size(); i++)
            {
                lowerName.assign(structs[i].name);
                utils::MakeStringLowerCase(lowerName);

                if (lowerName == "root")
                    return;
            }

            DefLogicError("Value cannot be negative");
        }
    };

    class JsonDDLRoot
    {
    public:
        std::vector<std::string> defFiles;
		std::vector<JsonDDLDef> defs;
    };

    namespace DDL
    {    
        bool IsStandardSize(const size_t size, const ddlPrimitiveTypes_e type, const size_t arraySize)
        {
            const auto memberSize = (size / arraySize);
            if (type < DDL_BYTE_TYPE || type > DDL_UINT64_TYPE)
                return false;
            return DDL_TYPE_FEATURES[type].size == memberSize;
        }

        std::string PermissionTypeToName(const ddlPermissionTypes_e type)
        {
            if (type <= DDL_PERM_UNSPECIFIED || type >= DDL_PERM_COUNT)
                return static_cast<int>(type) + "_unknown";

            return DDL_PERM_NAMES[type];
        }

        std::string TypeToName(const ddlPrimitiveTypes_e type)
        {
            if (type >= DDL_TYPE_COUNT)
                return static_cast<int>(type) + "_unknown";

            return DDL_TYPE_NAMES[type];
        }

        ddlPrimitiveTypes_e NameToType(const std::string& typeName)
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

        ddlPermissionTypes_e NameToPermissionType(const std::string& typeName)
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
    }
}
