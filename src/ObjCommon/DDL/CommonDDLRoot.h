#pragma once
#include "CommonDDL.h"
#include "CommonDDLMember.h"
#include "CommonDDLStruct.h"
#include "CommonDDLEnum.h"
#include "CommonDDLDef.h"

#include <iostream>

class CommonDDLRoot
{
public:
    const std::string m_root_name;
    // multiple defs can exist in a single file canonically
    // defs should be sorted based on version when linked
    std::unordered_map<std::string /*m_def_file*/, std::vector<CommonDDLDef>> m_defs;
    std::unordered_map<std::string /*m_def_file*/, std::vector<CommonDDLDef>> m_include_pool; // this could result in redundant copies of the same includedef for each def

    CommonDDLRoot(std::string& name);
};