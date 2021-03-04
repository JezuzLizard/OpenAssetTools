#pragma once

#include <unordered_map>

#include "ICodeTemplate.h"
#include "ZoneCodeGeneratorArguments.h"

class CodeGenerator
{
    const ZoneCodeGeneratorArguments* m_args;

    std::unordered_map<std::string, std::unique_ptr<ICodeTemplate>> m_template_mapping;

    void SetupTemplates();

    bool GenerateCodeForTemplate(RenderingContext* context, ICodeTemplate* codeTemplate) const;
    static bool GetAssetWithName(IDataRepository* repository, const std::string& name, StructureInformation*& asset);

public:
    explicit CodeGenerator(const ZoneCodeGeneratorArguments* args);

    bool GenerateCode(IDataRepository* repository);
};
