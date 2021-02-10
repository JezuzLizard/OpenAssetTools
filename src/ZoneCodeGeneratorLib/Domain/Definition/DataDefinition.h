#pragma once
#include <string>

#include "Utils/ClassUtils.h"

enum class DataDefinitionType
{
    STRUCT,
    UNION,
    ENUM,
    TYPEDEF,
    BASE_TYPE,
    FORWARD_DECLARATION
};

class DataDefinition
{
public:
    DataDefinition() = default;
    DataDefinition(std::string _namespace, std::string name);

    virtual ~DataDefinition() = default;
    DataDefinition(const DataDefinition& other) = default;
    DataDefinition(DataDefinition&& other) noexcept = default;
    DataDefinition& operator=(const DataDefinition& other) = default;
    DataDefinition& operator=(DataDefinition&& other) noexcept = default;

    std::string m_namespace;
    std::string m_name;
    
    _NODISCARD virtual DataDefinitionType GetType() const = 0;
    _NODISCARD virtual unsigned GetAlignment() = 0;
    _NODISCARD virtual bool GetForceAlignment() = 0;
    _NODISCARD virtual unsigned GetSize() = 0;

    _NODISCARD std::string GetFullName() const;
};
