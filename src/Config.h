#include <toml++/toml.hpp>

#include <string>
#include <filesystem>

#include "Logging.h"

namespace fs = std::filesystem;

struct ConfigParameters
{
	fs::path m_input;
	fs::path m_output;
	std::vector<std::string> m_filename_masks;
};

std::optional<ConfigParameters> processTomlFile(const std::string& file);

std::optional<ConfigParameters> processTomlString(const std::string& toml_str);

std::optional<ConfigParameters> processTomlTable(toml::table tbl);

