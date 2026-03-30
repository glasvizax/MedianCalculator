#include "Config.h"

void postProcessConfigParams(ConfigParams& params)
{
	LOG_DEBUG("postProcessConfigParams(): begin");
	LOG_TRACE("Input before normalize: '{}'", params.m_input.generic_string());
	LOG_TRACE("Output before normalize: '{}'", params.m_output.generic_string());

	std::error_code errcode;

	// If input path is relative, convert it to absolute
	// to avoid dependency on current working directory later.
	if (params.m_input.is_relative())
	{
		LOG_DEBUG("Input path is relative, converting to absolute: '{}'", params.m_input.generic_string());
		params.m_input = fs::absolute(params.m_input, errcode);
		ERRCODE_AUTOLOG(errcode);
		LOG_TRACE("Input after normalize: '{}'", params.m_input.generic_string());
	}
	else
	{
		LOG_TRACE("Input path is already absolute: '{}'", params.m_input.generic_string());
	}

	// If output is not specified, use default directory: current_path()/output.
	// Otherwise normalize it to absolute path as well.
	if (params.m_output.empty())
	{
		params.m_output = fs::current_path() / "output";
		LOG_DEBUG("Output path was empty, defaulting to '{}'", params.m_output.generic_string());
	}
	else if (params.m_output.is_relative())
	{
		LOG_DEBUG("Output path is relative, converting to absolute: '{}'", params.m_output.generic_string());
		params.m_output = fs::absolute(params.m_output);
		LOG_TRACE("Output after normalize: '{}'", params.m_output.generic_string());
	}
	else
	{
		LOG_TRACE("Output path is already absolute: '{}'", params.m_output.generic_string());
	}

	// Final result file name is always fixed.
	params.m_output /= "median_result.csv";
	LOG_INFO("Final output file path: '{}'", params.m_output.generic_string());

	LOG_DEBUG("postProcessConfigParams(): done");
}

std::optional<ConfigParams> processTomlFile(const fs::path& file)
{
	LOG_INFO("Reading TOML config file: '{}'", file.generic_string());

	toml::parse_result res = toml::parse_file(file.generic_string());

	if (res.failed())
	{
		LOG_CRITICAL("TOML parse failed for file '{}': {}", file.generic_string(), res.error().description());
		return std::nullopt;
	}

	LOG_DEBUG("TOML file parsed successfully: '{}'", file.generic_string());
	return processTomlTable(std::move(res).table());
}

std::optional<ConfigParams> processTomlString(const std::string& toml_str)
{
	LOG_DEBUG("Parsing TOML from string, size = {} bytes", toml_str.size());

	toml::parse_result res = toml::parse(toml_str);

	if (res.failed())
	{
		LOG_CRITICAL("TOML parse failed from string: {}", res.error().description());
		return std::nullopt;
	}

	LOG_DEBUG("TOML string parsed successfully");
	return processTomlTable(std::move(res).table());
}

std::optional<ConfigParams> processTomlTable(toml::table tbl)
{
	LOG_DEBUG("processTomlTable(): begin");

	auto input = tbl["main"]["input"];

	// Validate required field: main.input must exist and be a string.
	if (!input || !input.is_string())
	{
		LOG_ERROR("\"main.input\" must be specified as string");
		return std::nullopt;
	}

	std::optional<ConfigParams> config_params;
	config_params.emplace();

	config_params->m_input = std::move(input.ref<std::string>());
	LOG_INFO("Config: input='{}'", config_params->m_input.generic_string());

	auto output = tbl["main"]["output"];

	// Optional field: main.output
	if (output)
	{
		if (!output.is_string())
		{
			LOG_ERROR("\"main.output\" must be specified as string");
			return std::nullopt;
		}

		config_params->m_output = std::move(output.ref<std::string>());
		LOG_INFO("Config: output='{}'", config_params->m_output.generic_string());
	}
	else
	{
		LOG_DEBUG("\"main.output\" is not specified; default will be used");
	}

	auto filename_masks = tbl["main"]["filename_mask"];

	// Optional field: main.filename_mask (array of strings)
	if (filename_masks)
	{
		if (!filename_masks.is_array())
		{
			LOG_ERROR("\"main.filename_mask\" must be an array");
			return std::nullopt;
		}

		toml::array* arr = filename_masks.as_array();
		config_params->m_filename_masks.reserve(arr->size());

		LOG_DEBUG("Parsing {} filename mask(s)", arr->size());

		for (size_t i = 0; i < arr->size(); ++i)
		{
			auto& el = (*arr)[i];

			// Validate that every element is a string.
			if (!el.is_string())
			{
				LOG_ERROR("\"main.filename_mask\" must contain strings only (invalid element at index {})", i);
				return std::nullopt;
			}

			auto mask = std::move(el.ref<std::string>());
			LOG_TRACE("filename_mask[{}] = '{}'", i, mask);
			config_params->m_filename_masks.emplace_back(std::move(mask));
		}

		LOG_INFO("Loaded {} filename mask(s)", config_params->m_filename_masks.size());
	}
	else
	{
		LOG_DEBUG("\"main.filename_mask\" is not specified; default behavior will be used");
	}

	LOG_DEBUG("processTomlTable(): success");
	return config_params;
}

std::optional<fs::path> parseArgvForConfigPath(int argc, const char* const* argv)
{
	LOG_DEBUG("parseArgvForConfigPath(): argc = {}", argc);

	fs::path config_path;

	try
	{
		po::options_description desc;

		desc.add_options()
			("config", po::value<fs::path>(), "toml config file path")
			("cfg", po::value<fs::path>(), "short version of config");

		// Restrict parsing to [-arg value] format only.
		// This avoids ambiguous or unexpected command line forms.
		int style =
			(po::command_line_style::default_style | po::command_line_style::allow_long_disguise)
			& ~(po::command_line_style::allow_guessing | po::command_line_style::allow_long);

		po::command_line_parser parser(argc, argv);
		parser.options(desc).style(style).allow_unregistered();

		po::parsed_options parse_result = parser.run();
		LOG_TRACE("Command line parsed: {} option(s)", parse_result.options.size());

		if (!parse_result.options.empty())
		{
			auto unrec = po::collect_unrecognized(
				parse_result.options,
				po::collect_unrecognized_mode::include_positional
			);

			// Only one config option is allowed.
			// Any extra arguments are treated as an error.
			if (parse_result.options.size() > 1 || unrec.size())
			{
				LOG_ERROR("Use only one option: [-cfg path_to_toml_file] or [-config path_to_toml_file]");
				return std::nullopt;
			}

			po::variables_map vm;
			po::store(parse_result, vm);
			po::notify(vm);

			const auto& opt_name = parse_result.options[0].string_key;
			LOG_TRACE("Detected config option key: '{}'", opt_name);

			if (auto it = vm.find(opt_name); it != vm.end())
			{
				boost::any& any = it->second.value();

				// Safe extraction of fs::path from boost::any
				fs::path* cast_res = boost::any_cast<fs::path>(&any);
				if (!cast_res)
				{
					LOG_ERROR("Incorrect config file name type for option '{}'", opt_name);
					return std::nullopt;
				}

				config_path = std::move(*cast_res);
				LOG_INFO("Config path received from argv: '{}'", config_path.generic_string());
			}
			else
			{
				LOG_WARN("Config option key '{}' was not found in variables_map", opt_name);
			}
		}
		else
		{
			LOG_DEBUG("No config path argument provided; default config will be used");
		}
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Argument parsing error: {}", e.what());
		return std::nullopt;
	}

	LOG_DEBUG("parseArgvForConfigPath(): done");
	return config_path;
}

fs::path findTomlConfigFile(int argc, const char* const* argv)
{
	LOG_INFO("Searching for TOML config file");

	std::optional<fs::path> config_file = parseArgvForConfigPath(argc, argv);

	if (config_file == std::nullopt)
	{
		LOG_CRITICAL("Failed to obtain config file path from arguments");
		std::exit(EXIT_FAILURE);
	}

	// If no path provided via CLI, fallback to default config file.
	if (config_file->empty())
	{
		config_file = fs::current_path() / "config.toml";
		LOG_DEBUG("Config path was empty, using default: '{}'", config_file->generic_string());
	}

	std::error_code errcode;

	bool exist = fs::exists(*config_file, errcode);
	ERRCODE_AUTOLOG(errcode);

	if (!exist)
	{
		LOG_CRITICAL("Can't find config file: '{}'", config_file->generic_string());
		std::exit(EXIT_FAILURE);
	}

	LOG_INFO("Config file path resolved to: '{}'", config_file->generic_string());

	return *config_file;
}

ConfigParams receiveConfigParams(int argc, const char* const* argv)
{
	LOG_INFO("Receiving configuration parameters");

	fs::path config_file_path = findTomlConfigFile(argc, argv);

	auto config_parameters = processTomlFile(config_file_path);

	if (!config_parameters)
	{
		LOG_CRITICAL("Failed to build ConfigParams from TOML file '{}'", config_file_path.generic_string());
		std::exit(EXIT_FAILURE);
	}

	postProcessConfigParams(*config_parameters);

	LOG_INFO("Configuration successfully loaded");
	LOG_DEBUG("receiveConfigParams(): done");

	return *config_parameters;
}