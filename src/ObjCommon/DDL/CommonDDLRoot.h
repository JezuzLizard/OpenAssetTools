#pragma once
#include "CommonDDL.h"
#include "CommonDDLMember.h"
#include "CommonDDLStruct.h"
#include "CommonDDLEnum.h"
#include "CommonDDLDef.h"

#include <iostream>

struct File
{
    std::string name;                            // The name of the file
    std::vector<std::shared_ptr<File>> includes; // The list of included files

    File(const std::string& name)
        : name(name)
    {
    }

    void AddInclude(const std::shared_ptr<File>& file)
    {
        includes.push_back(file);
    }

    void PrintIncludes(auto depth = 0u) const
    {
        std::string message = "";
        for (int i = 0u; i < depth; ++i)
        {
            message += " ";
        }

        std::cout << message + name << std::endl;
        for (const auto& include : includes)
        {
            include->PrintIncludes(depth + 1u);
        }
    }
};

std::shared_ptr<File> resolveFile(const std::string& name, std::unordered_map<std::string, std::shared_ptr<File>>& fileMap)
{
    if (fileMap.find(name) == fileMap.end())
    {
        fileMap[name] = std::make_shared<File>(name);
    }
    return fileMap[name];
}

class CommonDDLRoot
{
public:
    std::string m_root_name;
    std::unordered_map<std::string /*m_def_file*/, std::vector<CommonDDLDef>> m_defs; // multiple defs can exist in a single file canonically
};