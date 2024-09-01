#pragma once

#include "CommonDDLDef.h"
#include "Utils/ClassUtils.h"

#include <string>
#include <vector>

class CommonDDLRoot
{
public:
    std::vector<DDLString> m_def_files;
    std::vector<CommonDDLDef> m_defs;
};