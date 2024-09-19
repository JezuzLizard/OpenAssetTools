#pragma once
#include "Dumping/AbstractTextDumper.h"
#include "DDL/CommonDDLRoot.h"

#include <ostream>
#include <string>
#include <vector>

class DDLDumper : AbstractTextDumper
{
    struct
    {
        bool m_empty_line_before_definition : 1;
    } m_flags;

    void WriteLineComment(const std::string& comment) const;

    void DumpEnum(const CommonDDLEnumDef& _enum);
    void DumpStruct(const CommonDDLDef& def, const CommonDDLStructDef& _struct, size_t structIndex);

public:
    explicit DDLDumper(std::ostream& stream);

    void DumpDef(const CommonDDLDef& def);
};
