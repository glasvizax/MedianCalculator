#pragma once

#include "Logging.h"
#include "Config.h"

std::set<fs::path> findMatchingCsvFiles(ConfigParams config_params);

bool validateCsvPath(const std::vector<std::string>& masks, const fs::path& file);