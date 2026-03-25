#include "Misc.h"

std::vector<fs::path> findMatchingCsvFiles(ConfigParams config_params)
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

    std::vector<fs::path> csv_files;
    for (; it != fs::recursive_directory_iterator{}; it.increment(errcode))
    {
        ERRCODE_AUTOLOG(errcode);

        if (it->is_regular_file())
        {
            fs::path file(*it);

            if (validateCsvPath(config_params.m_filename_masks, file))
            {
                csv_files.push_back(std::move(file));
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
        // there could be better algorithm
        if (file.stem().string().find(mask) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// first return - view before delim, second - after
std::pair<std::string_view, std::string_view> splitView(std::string_view view, char delim)
{
    size_t delim_idx = view.find(delim);
    if (delim_idx != std::string_view::npos) 
    {
        return std::make_pair(view.substr(0, delim_idx), view.substr(delim_idx + 1));
    }
    return std::make_pair(view, std::string_view());
}

bool safeGetline(std::istream& is, std::string& line)
{
    if (!std::getline(is, line))
    {
        return false;
    }

    // (Windows CRLF)
    if (!line.empty() && line.back() == '\r') 
    {
        line.pop_back();
    }

    return true;
}


//TODO : impove lifescope
static std::vector<std::string> s_string_buffers;
std::mutex string_buffer_mtx;

std::string popStringBuffer()
{
    {
        std::scoped_lock lock(string_buffer_mtx);
        if (!s_string_buffers.empty())
        {
            std::string buff = std::move(s_string_buffers.back());
            s_string_buffers.pop_back();
            return buff;
        }
    }
    std::string buff;
    buff.reserve(512);
    return buff;
}

void pushStringBuffer(std::string&& buff)
{
    std::scoped_lock lock(string_buffer_mtx);
    if (buff.capacity() > 0)
    {
        buff.clear();
        s_string_buffers.push_back(std::move(buff));
    }
}
