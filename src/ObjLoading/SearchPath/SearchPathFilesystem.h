#pragma once

#include "ISearchPath.h"
#include <string>

class SearchPathFilesystem final : public ISearchPath
{
    std::string m_path;

public:
    explicit SearchPathFilesystem(std::string path);

    FileAPI::IFile* Open(const std::string& fileName) override;
    std::string GetPath() override;
    void Find(const SearchPathSearchOptions& options, const std::function<void(const std::string&)>& callback) override;
};