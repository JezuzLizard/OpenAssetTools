#include "JsonDDLRoot.h"

#include "Game/T6/CommonT6.h"

#include <iostream>
#include <unordered_set>

#include "Utils/StringUtils.h"

namespace T6
{
    inline const std::unordered_map<std::string, int> DDL_TYPE_NAMES = {
        {"byte", DDL_BYTE_TYPE},
         {"short", DDL_SHORT_TYPE},
         {"uint", DDL_UINT_TYPE},
         {"int", DDL_INT_TYPE},
         {"uint64", DDL_UINT64_TYPE},
         {"float", DDL_FLOAT_TYPE},
         {"fixed_float", DDL_FIXEDPOINT_TYPE},
         {"string", DDL_STRING_TYPE},
         {"struct", DDL_STRUCT_TYPE},
         {"enum", DDL_ENUM_TYPE},
         {"", DDL_TYPE_COUNT}
    };

    inline const std::unordered_map<std::string, int> DDL_PERM_NAMES = {
        {"unspecified", DDL_PERM_UNSPECIFIED},
        {"client", DDL_PERM_CLIENT},
        {"server", DDL_PERM_SERVER},
        {"both", DDL_PERM_BOTH},
        {"", DDL_PERM_COUNT}
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

    //This isn't strictly necessary, but I'm doing this anyway just in case someone wanted to transpile to the official formats.
    //By maintaining the same standards the canonical DDL linking that the T7 mod tools linker has, avoids potential future issues if someone wanted to link a dumped DDL dumped using the json code here. 
    inline const std::unordered_set<std::string> DDL_KEYWORDS = {
        "struct",
        /*"root",*/
        "enum",
        "version",
        "bool",
        "byte",
        "short",       
        "uint",        
        "int", 
        "uint64",
        "float",
        "fixed",
        "string",
        "codeversion"
        "checksum",
        "nopadding",
        "userflags",
        "ddlchecksum",
        "reserve",
        "server",
        "client",
        "both",
        "",
    };

    JsonDDLParseException::JsonDDLParseException(std::string& message)
        : std::runtime_error(message)
    {
    }

    NameException::NameException(std::string& message)
        : JsonDDLParseException::JsonDDLParseException(message)
    {
    }

    void NameError(const std::string& message)
    {
        std::string prefaceAndMessage = std::format("") + message;
        throw NameException::NameException(prefaceAndMessage);
    }

    void ValidateName(const std::string& name)
    {
        if (name.empty())
            NameError("name field cannot be empty");

        if (!std::isalpha(name[0]))
            NameError(std::format("name field cannot begin with \"{}\"", name[0]));

        for (auto i = 1u; i < name.length(); i++)
        {
            if (!std::isalnum(name[i]) && name.substr(i, 1) != "_")
                NameError(std::format("name field cannot contain \"{}\"", name[i]));
        }

        const auto& it = DDL_KEYWORDS.find(name);
        if (it != DDL_KEYWORDS.end())
            NameError(std::format("name field cannot be reserved \"{}\"", it->data()));
    }

    JsonDDLMemberDef::Exception::Exception(std::string& message)
        : JsonDDLParseException::JsonDDLParseException(message)
    {
    }

    void JsonDDLMemberDef::LogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Member: {} Type: {} Struct {}: ", name, this->TypeToName(), parentStruct) + message;
        throw JsonDDLMemberDef::Exception(prefaceAndMessage);
    }

    const bool JsonDDLMemberDef::IsStandardSize() const
    {
        if (type.value() < DDL_BYTE_TYPE || type.value() > DDL_UINT64_TYPE)
            return false;
        return DDL_TYPE_FEATURES[type.value()].size == size.value() / arraySize.value_or(1);
    }

    const std::string& JsonDDLMemberDef::PermissionTypeToName() const noexcept
    {
        for (const auto& [k, v] : DDL_PERM_NAMES)
        {
            if (v == type.value())
                return k;
        }

        return static_cast<int>(type.value()) + "_unknown";
    }

    const std::string& JsonDDLMemberDef::TypeToName() const noexcept
    {
        for (const auto& [k, v] : DDL_TYPE_NAMES)
        {
            if (v == type.value())
                return k;
        }

        return static_cast<int>(type.value()) + "_unknown";
    }
    
    ddlPrimitiveTypes_e JsonDDLMemberDef::NameToType(const std::string& typeName) const noexcept
    {
        std::string copy = typeName;
        utils::MakeStringLowerCase(copy);

        auto it = DDL_TYPE_NAMES.find(copy);
        if (it != DDL_TYPE_NAMES.end())
            return static_cast<ddlPrimitiveTypes_e>(it->second);

        return DDL_TYPE_COUNT;
    }

    ddlPermissionTypes_e JsonDDLMemberDef::NameToPermissionType(const std::string& typeName) const noexcept
    {
        std::string copy = typeName;
        utils::MakeStringLowerCase(copy);

        auto it = DDL_TYPE_NAMES.find(copy);
        if (it != DDL_TYPE_NAMES.end())
            return static_cast<ddlPermissionTypes_e>(it->second);

        return DDL_PERM_UNSPECIFIED;
    }

    void JsonDDLMemberDef::ReportCircularDependency(const JsonDDLDef& jDDLDef, std::string message) const
    {
        std::string traceback("Traceback:\n");
        for (auto i = 0u; i < jDDLDef.memberStack.size(); i++)
        {
            traceback += std::format("\tName: {}, Type: {} Parent: {}\n",
                                     jDDLDef.memberStack[i].name,
                                     jDDLDef.memberStack[i].struct_.has_value() ? jDDLDef.memberStack[i].struct_.value() : jDDLDef.memberStack[i].TypeToName(),
                                     jDDLDef.memberStack[i].parentStruct);
        }
        std::string prefaceAndMessage = std::format("{}\n{}", message, traceback);
        throw JsonDDLMemberDef::Exception(prefaceAndMessage);
    }

    void JsonDDLMemberDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        referenceCount++;
        if (calculated)
            return;

        if (limits.has_value())
        {
            ValidateBitFields();
            ValidateRange();
            ValidateFixedPoint();
        }

        ValidateMaxCharacters(jDDLDef);

        if (enum_.has_value())
        {
            auto index = jDDLDef.TypeToEnumIndex(enum_.value());
            if (!index.has_value())
            {
                LogicError("could not create enumIndex for _enum field");
            }
            auto& enumDef = jDDLDef.enums.at(index.value());
            enumDef.Validate(jDDLDef);
        }

        auto index = jDDLDef.TypeToStructIndex(this->TypeToName());
        if (index.has_value())
        {
            if (index.value() == 0)
                LogicError("root struct cannot be used as a type");
            if (jDDLDef.inCalculation.at(index.value()))
                ReportCircularDependency(jDDLDef, "circular dependency detected");

            const auto& structDef = jDDLDef.structs.at(index.value());
            jDDLDef.inCalculation[index.value()] = true;
            jDDLDef.memberStack.push_back(*this);
            structDef.Validate(jDDLDef);
            jDDLDef.memberStack.pop_back();
        }

        ValidateName(jDDLDef);
        ValidateType(jDDLDef);
        ValidatePermission(jDDLDef);
        ValidateArray(jDDLDef);
        ValidateEnum_(jDDLDef);

        calculated = true;
        jDDLDef.inCalculation.clear();
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

        auto calculatedSize = 0;
        if (type.value() <= DDL_FIXEDPOINT_TYPE)
        {
            if (!limits.has_value())
                calculatedSize = DDL_TYPE_FEATURES[type.value()].size;
            else if (limits.value().bits.has_value())
                calculatedSize = limits.value().bits.value();
            else if (limits.value().range.has_value())
                calculatedSize = std::bit_width(limits.value().range.value());
            else if (limits.value().fixedMagnitudeBits.has_value() && limits.value().fixedPrecisionBits.has_value())
            {
                limits.value().range.emplace(std::bit_width(limits.value().fixedPrecisionBits.value()));
                calculatedSize = limits.value().fixedMagnitudeBits.value() + limits.value().fixedPrecisionBits.value();
            }
        }
        else if (type.value() == DDL_STRING_TYPE)
        {
            if (maxCharacters.has_value())
                calculatedSize = maxCharacters.value() * CHAR_BIT;
        }
        else if (type.value() == DDL_STRUCT_TYPE)
        {
            if (externalIndex.has_value())
            {
                if (jDDLDef.inCalculation.at(externalIndex.value()))
                    LogicError("circular dependency detected");

                auto& structDef = jDDLDef.structs.at(externalIndex.value());
                jDDLDef.inCalculation[externalIndex.value()];
                structDef.Calculate(jDDLDef);
                calculatedSize = structDef.size.value();
            }
        }

        if (!calculatedSize)
            LogicError("could not calculate size");

        calculated = true;
        jDDLDef.inCalculation.clear();

        size.emplace(calculatedSize * arraySize.value_or(1));
    }

    void JsonDDLMemberDef::ValidateName(const JsonDDLDef& jDDLDef) const
    {
        std::string primary = name;
        utils::MakeStringLowerCase(primary);
        std::string other;

        T6::ValidateName(primary);

        size_t nameRedefinitions = 0;
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            other.assign(jDDLDef.structs[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                LogicError("member cannot have the same name as a struct");
        }

        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            other.assign(jDDLDef.enums[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                LogicError("member cannot have the same name as an enum");
        }
    }

    void JsonDDLMemberDef::ValidateType(const JsonDDLDef& jDDLDef) const
    {
        if (type <= DDL_STRING_TYPE)
            return;
        if (type < DDL_TYPE_COUNT)
            LogicError("type cannot directly be defined as enum or struct");

        std::string lowerName1 = name;
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
                LogicError("type field cannot be set to an enum, the enum_ field is required instead");
        }

        LogicError("no definition found for type value");
    }

    void JsonDDLMemberDef::ValidatePermission(const JsonDDLDef& jDDLDef) const
    {
        if (permission <= DDL_PERM_UNSPECIFIED || permission >= DDL_PERM_COUNT)
            LogicError("permission must be client, server, or both");
    }

    void JsonDDLMemberDef::ValidateArray(const JsonDDLDef& jDDLDef) const
    {
        if (!arraySize.has_value())
            return;
        if (enum_.has_value())
            LogicError("arraySize field cannot be combined with enum_ field");
        if (arraySize.value() <= 0)
            LogicError("arraySize cannot be 0");
    }

    void JsonDDLMemberDef::ValidateMaxCharacters(const JsonDDLDef& jDDLDef) const
    {
        if (type == DDL_STRING_TYPE)
        {
            if (!maxCharacters.has_value())
                LogicError("string type requires maxCharacters field");
            if (maxCharacters.value() <= 0)
                LogicError("maxCharacters cannot be 0");

            return;
        }
        else if (!maxCharacters.has_value())
            return;

        LogicError("maxCharacters field cannot be used with types other than string");
    }

    void JsonDDLMemberDef::ValidateEnum_(const JsonDDLDef& jDDLDef) const
    {
        if (!enum_.has_value())
            return;
        if (arraySize.has_value())
            LogicError("arrayCount field cannot be combined with enum_ field");

        std::string lowerName1 = name;
        utils::MakeStringLowerCase(lowerName1);
        std::string lowerName2;
        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            lowerName2.assign(jDDLDef.enums[i].name);
            if (lowerName1 != lowerName2)
                continue;

            return;
        }

        LogicError("no definition for enum");
    }

    void JsonDDLMemberDef::ValidateBitFields() const
    {
        if (!limits->bits.has_value())
            return;

        if ((DDL_TYPE_FEATURES[type.value()].flags & DDL_FLAG_BITFIELDS) == 0)
            LogicError("type does not support bits field");
        if (limits->range.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
            LogicError("range, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with bits field");
        if (limits->bits.value() > DDL_TYPE_FEATURES[type.value()].size)
            LogicError("bits exceeds maximum possible bits for type");
    }

    void JsonDDLMemberDef::ValidateRange() const
    {
        if (!limits->range.has_value())
            return;

        if ((DDL_TYPE_FEATURES[type.value()].flags & DDL_FLAG_LIMITS) == 0)
            LogicError("does not support range field");
        if (limits->bits.has_value() || limits->fixedPrecisionBits.has_value() || limits->fixedMagnitudeBits.has_value())
            LogicError("bits, fixedPrecisionBits, and fixedMagnitudeBits cannot be combined with range field");
        if (limits->range.value() > DDL_TYPE_FEATURES[type.value()].max)
            LogicError("range exceeds maximum possible value for type");
    }

    void JsonDDLMemberDef::ValidateFixedPoint() const
    {
        if (!limits->fixedMagnitudeBits.has_value() && !limits->fixedPrecisionBits.has_value())
            if (type == DDL_FIXEDPOINT_TYPE)
                LogicError("fixed_float requires both fixedMagnitudeBits, and fixedPrecisionBits fields");
            return;

        if (type != DDL_FIXEDPOINT_TYPE)
            LogicError("type must be fixed_float in order to use fixedMagnitudeBits, and FixedPrecisionBits");
        if (limits->range.has_value() || limits->bits.has_value())
            LogicError("range, and bits fields cannot be used with fixed_float type");
        if ((limits->fixedMagnitudeBits.value() + limits->fixedPrecisionBits.value()) > DDL_TYPE_FEATURES[type.value()].size)
            LogicError("magnitude, and precision bits combined cannot exceed 32 bits");
    }

    void JsonDDLStructDef::LogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef) + message;
        throw JsonDDLStructDef::Exception(prefaceAndMessage);
    }

    void JsonDDLStructDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        if (parentDef.empty())
            parentDef.assign(jDDLDef.filename);
        ValidateName(jDDLDef);
        ValidateMembers(jDDLDef);
    }

    void JsonDDLStructDef::CalculateHashes()
    {
        for (auto i = 0; i < members.size(); i++)
        {
            const auto hash = Common::Com_HashString(members[i].name.c_str());
            hashTable.emplace(hash, i);
        }
    }

    void JsonDDLStructDef::Calculate(JsonDDLDef& jDDLDef)
    {
        referenceCount++;
        if (calculated)
            return;

        this->CalculateHashes();

        auto size = 0u;
        for (auto i = 0u; i < members.size(); i++)
        {
            members[i].Calculate(jDDLDef);
            members[i].offset.emplace(size);
            size += members[i].size.value();
        }

        this->size.emplace(size);

        calculated = true;
    }

    JsonDDLStructDef::Exception::Exception(std::string& message)
        : JsonDDLParseException::JsonDDLParseException(message)
    {
    }

    void JsonDDLStructDef::ValidateName(const JsonDDLDef& jDDLDef) const
    {
        std::string primary = name;
        utils::MakeStringLowerCase(primary);
        std::string other;

        T6::ValidateName(primary);

        size_t nameRedefinitions = 0;
        for (auto i = 0u; i < jDDLDef.structs.size(); i++)
        {
            other.assign(jDDLDef.structs[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                LogicError("multiple redefinitions of struct");
        }

        for (auto i = 0u; i < jDDLDef.enums.size(); i++)
        {
            other.assign(jDDLDef.enums[i].name);
            utils::MakeStringLowerCase(other);
            if (other == primary)
                nameRedefinitions++;

            if (nameRedefinitions > 1)
                LogicError("struct cannot have same name as enum");
        }
    }

    void JsonDDLStructDef::ValidateMembers(const JsonDDLDef& jDDLDef) const
    {
        if (members.empty())
            LogicError("struct must have at least one member");

        for (const auto& member : members)
        {
            member.parentStruct.assign(name);
            member.Validate(jDDLDef);
        }
    }

    void JsonDDLStructDef::ReferenceCount(const JsonDDLDef& jDDLDef) const
    {
        if (!referenceCount)
            LogicError("an unreferenced struct cannot be linked");
    }

    void JsonDDLEnumDef::LogicError(const std::string& message) const
    {
        std::string prefaceAndMessage = std::format("DDL Struct: {} Def: ", name, parentDef) + message;
        throw JsonDDLEnumDef::Exception(prefaceAndMessage);
    }

    void JsonDDLEnumDef::Validate(const JsonDDLDef& jDDLDef) const
    {
        ValidateMembers(jDDLDef);
        ValidateName(jDDLDef);
    }

    void JsonDDLEnumDef::CalculateHashes()
    {
        referenceCount++;
        if (calculated)
            return;

        for (auto i = 0; i < members.size(); i++)
        {
            const auto hash = Common::Com_HashString(members[i].c_str());
            hashTable.emplace(hash, i);
        }

        calculated = true;
    }

    JsonDDLEnumDef::Exception::Exception(std::string& message)
        : JsonDDLParseException::JsonDDLParseException(message)
    {
    }

    void JsonDDLEnumDef::ValidateName(const JsonDDLDef& jDDLDef) const
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

            LogicError(std::format("multiple occurrences of enum member \"{}\"", members[i]));
        }
    }

    void JsonDDLEnumDef::ValidateMembers(const JsonDDLDef& jDDLDef) const
    {
        if (members.empty())
            LogicError("enum must have at least one member");
    }

    void JsonDDLEnumDef::ReferenceCount(const JsonDDLDef& jDDLDef) const
    {
        if (!referenceCount)
            LogicError("an unreferenced enum cannot be linked");
    }

    void JsonDDLDef::LogicError(const std::string& message) const
    {
        assert(!filename.empty());

        std::string prefaceAndMessage = std::format("DDL Def: {} ", filename) + message;
        throw JsonDDLDef::Exception(prefaceAndMessage);
    }

    std::optional<size_t> JsonDDLDef::TypeToStructIndex(const std::string& typeName) const
    {
        std::string lowerType = typeName;
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

    std::optional<size_t> JsonDDLDef::TypeToEnumIndex(const std::string& typeName) const
    {
        std::string lowerType = typeName;
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

    std::optional<std::string> JsonDDLDef::StructIndexToType(const size_t index) const
    {
        if (structs.size() < index)
            return structs[index].name;

        return std::nullopt;
    }

    std::optional<std::string> JsonDDLDef::EnumIndexToType(const size_t index) const
    {
        if (enums.size() < index)
            return enums[index].name;

        return std::nullopt;
    }

    void JsonDDLDef::Validate() const
    {
        try
        {
            ValidateRoot();
        }
        catch (JsonDDLDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (JsonDDLEnumDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (JsonDDLStructDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (JsonDDLMemberDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (T6::NameException& e)
        {
            std::cerr << e.what() << "\n";
        }
    }

    void JsonDDLDef::Calculate()
    {
        try
        {
            for (auto& struc : structs)
            {
                std::string lowerName = struc.name;
                utils::MakeStringLowerCase(lowerName);
                if (lowerName != "root")
                {
                    continue;
                }

                struc.Calculate(*this);
                return;
            }

            LogicError("root struct entry point missing");
        }
        catch (JsonDDLDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (JsonDDLEnumDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (JsonDDLStructDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (JsonDDLMemberDef::Exception& e)
        {
            std::cerr << e.what() << "\n";
        }
        catch (T6::NameException& e)
        {
            std::cerr << e.what() << "\n";
        }
    }

    JsonDDLDef::Exception::Exception(std::string& message)
        : JsonDDLParseException(message)
    {
    }

    void JsonDDLDef::ValidateRoot() const
    {
        std::string lowerName;
        auto i = 0u;
        for ( ; i < structs.size(); i++)
        {
            lowerName.assign(structs[i].name);
            utils::MakeStringLowerCase(lowerName);

            if (lowerName == "root")
                break;

            LogicError("root struct entry point missing");
        }

        structs[i].Validate(*this);

        for (const auto& struc : structs)
        {
            struc.ReferenceCount(*this);
        }

        for (const auto& enum_ : enums)
        {
            enum_.ReferenceCount(*this);
        }
    }
    
} // namespace T6
