#pragma once

#include "Logging.h"
#include "Config.h"

std::vector<fs::path> findMatchingCsvFiles(const ConfigParams& config_params);

bool validateCsvPath(const std::vector<std::string>& masks, const fs::path& file);

std::string popStringBuffer();
void pushStringBuffer(std::string&& buff);

std::pair<std::string_view, std::string_view> splitView(std::string_view view, char delim = ';');

bool safeGetline(std::istream& is, std::string& line);

struct StringLease
{
    StringLease()
    {
        buff = popStringBuffer();
    }

    StringLease(const StringLease&) = delete;
    StringLease& operator=(const StringLease&) = delete;

    std::string& str()
    {
        return buff;
    }

    ~StringLease()
    {
        pushStringBuffer(std::move(buff));
    }

private:
    std::string buff;
};
