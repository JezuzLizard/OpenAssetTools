#pragma once

#include <map>

#include "Persistence/IDataRepository.h"

class InMemoryRepository final : public IDataRepository
{
    std::vector<EnumDefinition*> m_enums;
    std::vector<StructDefinition*> m_structs;
    std::vector<UnionDefinition*> m_unions;
    std::vector<TypedefDefinition*> m_typedefs;
    std::vector<StructureInformation*> m_structures_information;
    std::vector<FastFileBlock*> m_fast_file_blocks;
    std::map<std::string, DataDefinition*> m_data_definitions_by_name;
    std::map<DefinitionWithMembers*, StructureInformation*> m_structure_information_by_definition;

public:
    void Add(std::unique_ptr<EnumDefinition> enumsDefinition) override;
    void Add(std::unique_ptr<StructDefinition> structDefinition) override;
    void Add(std::unique_ptr<UnionDefinition> unionDefinition) override;
    void Add(std::unique_ptr<TypedefDefinition> typedefDefinition) override;
    void Add(std::unique_ptr<StructureInformation> structureInformation) override;
    void Add(std::unique_ptr<FastFileBlock> fastFileBlock) override;

    _NODISCARD const std::vector<EnumDefinition*>& GetAllEnums() const override;
    _NODISCARD const std::vector<StructDefinition*>& GetAllStructs() const override;
    _NODISCARD const std::vector<UnionDefinition*>& GetAllUnions() const override;
    _NODISCARD const std::vector<TypedefDefinition*>& GetAllTypedefs() const override;
    _NODISCARD const std::vector<StructureInformation*>& GetAllStructureInformation() const override;
    _NODISCARD const std::vector<FastFileBlock*>& GetAllFastFileBlocks() const override;

    _NODISCARD DataDefinition* GetDataDefinitionByName(const std::string& name) const override;
    _NODISCARD StructureInformation* GetInformationFor(DefinitionWithMembers* definitionWithMembers) const override;
};
