#include "Config.h"

std::optional<ConfigParameters> processTomlFile(const fs::path& file)
{
	toml::parse_result res = toml::parse_file(file.generic_string());

	if (res.failed())
	{
		LOG_ERROR(res.error().description());
		return std::nullopt;
	}

	return processTomlTable(std::move(res).table());
}

std::optional<ConfigParameters> processTomlString(const std::string& toml_str)
{
	toml::parse_result res = toml::parse(toml_str);

	if (res.failed())
	{
		LOG_ERROR(res.error().description());
		return std::nullopt;
	}

	return processTomlTable(std::move(res).table());
}

std::optional<ConfigParameters> processTomlTable(toml::table tbl)
{
	auto input = tbl["main"]["input"];

	if (!input || !input.is_string())
	{
		LOG_ERROR("\"input\ = \'dir_to_search_csv_files\'\" must be specified as string");
		return std::nullopt;
	}

	std::optional<ConfigParameters> config_params;
	config_params.emplace();

	config_params->m_input = std::move(input.ref<std::string>());

	auto output = tbl["main"]["output"];
	if (!output)
	{
		config_params->m_output = "output";
	}
	else
	{
		if (!output.is_string())
		{
			LOG_ERROR("\"output\ = \'dir_for_result_csv_file\'\" must be specified as string");
			config_params->m_output = "output";
		}
		else
		{
			config_params->m_output = std::move(output.ref<std::string>());
		}
	}

	auto filename_masks = tbl["main"]["filename_mask"];

	if (filename_masks)
	{
		if (!filename_masks.is_array())
		{
			LOG_WARN("\"filename_masks\" must be an array");

			if (!filename_masks.is_string())
			{
				LOG_WARN("\"filename_masks\" must contain strings");

			}
			else
			{
				config_params->m_filename_masks.emplace_back(std::move(filename_masks.ref<std::string>()));
			}
		}
		else
		{
			toml::array* arr = filename_masks.as_array();

			config_params->m_filename_masks.reserve(arr->size());

			for (auto& el : *arr)
			{
				if (!el.is_string())
				{
					LOG_WARN("\"filename_masks\" must contain strings");
					continue;
				}

				config_params->m_filename_masks.emplace_back(std::move(el.ref<std::string>()));
			}
		}
	}

	return config_params;
}

std::optional<fs::path> parseArgvForConfigPath(int argc, const char const* const* argv)
{
	fs::path config_path;

	try
	{
		po::options_description desc;

		desc.add_options()
			("config", po::value<fs::path>(), "toml config file path")
			("cfg", po::value<fs::path>(), "short version of config");

		// allow only for args of type [-arg val]
		int style = (po::command_line_style::default_style | po::command_line_style::allow_long_disguise) & ~(po::command_line_style::allow_guessing | po::command_line_style::allow_long);
		po::command_line_parser parser(argc, argv);
		parser.options(desc).style(style).allow_unregistered();

		po::parsed_options parse_result = parser.run();

		if (!parse_result.options.empty())
		{
			auto unrec = po::collect_unrecognized(parse_result.options, po::collect_unrecognized_mode::include_positional);
			// if received one unregistered or options given more than 1
			if (parse_result.options.size() > 1 || unrec.size())
			{
				LOG_ERROR("It can only be either [-cfg path_to_toml_file] or [-config path_to_toml_file]");
				return std::nullopt;
			}

			po::variables_map vm;
			po::store(parse_result, vm);
			po::notify(vm);

			if (auto it = vm.find(parse_result.options[0].string_key); it != vm.end())
			{
				boost::any& any = it->second.value();
				fs::path* cast_res = boost::any_cast<fs::path>(&any);
				if (!cast_res)
				{
					LOG_CRITICAL("incorrect config file name");
					return std::nullopt;
				}

				config_path = std::move(*cast_res);
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_CRITICAL("parse error: {}", e.what());
		return std::nullopt;
	}

	return config_path;
}

fs::path getConfigFile(int argc, const char const* const* argv)
{
	std::optional<fs::path> config_file = parseArgvForConfigPath(argc, argv);
	
	if(config_file == std::nullopt)
	{
		std::exit(EXIT_FAILURE);
	}

	if (config_file->empty())
	{
		config_file = fs::current_path() / "config.toml";
	}

	std::error_code err_code;

	bool exist = fs::exists(*config_file, err_code);

	if (err_code)
	{
		LOG_CRITICAL(err_code.message());
		std::exit(EXIT_FAILURE);
	}
	 
	if (!exist)
	{
		LOG_CRITICAL("can't find file {}", config_file->generic_string());
		std::exit(EXIT_FAILURE);
	}

	LOG_INFO("config file path = {}", config_file->generic_string());

	return *config_file;
}

