#include "Misc.h"

std::set<fs::path> findMatchingCsvFiles(ConfigParams config_params)
{
    std::error_code errcode;
    bool exist = fs::exists(config_params.m_input, errcode);

    ERRCODE_AUTOLOG(errcode);

    if (!exist)
    {
        LOG_ERROR("input path [{}] doesn't exist", config_params.m_input.generic_string());
        return {};
    }

    auto it = fs::recursive_directory_iterator(config_params.m_input, errcode);
    ERRCODE_AUTOLOG(errcode);

    std::set<fs::path> csv_files;
    for (; it != fs::recursive_directory_iterator{}; it.increment(errcode))
    {
        ERRCODE_AUTOLOG(errcode);

        if (it->is_regular_file())
        {
            fs::path file(*it);

            if (validateCsvPath(config_params.m_filename_masks, file))
            {
                csv_files.insert(std::move(file));
            }
        }
    }
    return csv_files;
}

bool validateCsvPath(const std::vector<std::string>& masks, const fs::path& file)
{
    if (file.extension() != ".csv")
    {
        return false;
    }

    for (auto& mask : masks)
    {
        // we check whether the file matches at least one mask.
        if (file.stem().generic_string().find(mask) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}