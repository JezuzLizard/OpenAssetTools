#pragma once
#include "CommonDDL.h"

class CommonDDLDef;

class CommonDDLRoot
{
public:
    std::vector<std::string> m_def_files;
    std::vector<CommonDDLDef> m_defs;
    std::vector<std::string> m_include_files;
    std::unordered_map<std::string, CommonDDLDef> m_includes;
};