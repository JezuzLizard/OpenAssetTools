#pragma once

#include "IHeaderBlock.h"

class HeaderBlockNamespace final : public IHeaderBlock
{
    std::string m_namespace_name;

public:
    explicit HeaderBlockNamespace(std::string namespaceName);

    HeaderBlockType GetType() override;
    const std::vector<sequence_t*>& GetTestsForBlock() override;
    void OnOpen(HeaderParserState* state) override;
    void OnClose(HeaderParserState* state) override;
    void OnChildBlockClose(HeaderParserState* state, IHeaderBlock* block) override;
};
