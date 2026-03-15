#include "Config.h"

std::optional<ConfigParameters> processTomlFile(const std::string& file)
{
	toml::parse_result res = toml::parse_file(file);

	if (res.failed())
	{
		LOG_ERROR(res.error().description());
		return std::optional<ConfigParameters>(std::nullopt);
	}

	return processTomlTable(std::move(res).table());
}

std::optional<ConfigParameters> processTomlString(const std::string& toml_str)
{
	toml::parse_result res = toml::parse(toml_str);

	if (res.failed())
	{
		LOG_ERROR(res.error().description());
		return std::optional<ConfigParameters>(std::nullopt);
	}

	return processTomlTable(std::move(res).table());
}

std::optional<ConfigParameters> processTomlTable(toml::table tbl)
{
	auto input = tbl["main"]["input"];

	if (!input || !input.is_string())
	{
		LOG_ERROR("\"input\ = \'dir_to_search_csv_files\'\" must be specified as string");
		return std::optional<ConfigParameters>(std::nullopt);
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
			LOG_WARN("\"output\ = \'dir_for_result_csv_file\'\" must be specified as string");
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
