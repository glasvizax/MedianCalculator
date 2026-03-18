#include <toml++/toml.hpp>

#include <string>
#include <filesystem>

#include "Logging.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;

struct ConfigParameters
{
	fs::path m_input;
	fs::path m_output;
	std::vector<std::string> m_filename_masks;
};

std::optional<ConfigParameters> processTomlFile(const fs::path& file);

std::optional<ConfigParameters> processTomlString(const std::string& toml_str);

std::optional<ConfigParameters> processTomlTable(toml::table tbl);

fs::path parseArgvForConfigPath(int argc, const char const* const* argv, bool& error);

fs::path getConfigFile(int argc, const char const* const* argv);
