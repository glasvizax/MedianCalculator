#include "Misc.h"

std::vector<fs::path> findMatchingCsvFiles(const ConfigParams& config_params)
{
    LOG_INFO("Scanning directory for CSV files: '{}'", config_params.m_input.generic_string());

    std::error_code errcode;

    // Check that input directory exists before iterating
    bool exist = fs::exists(config_params.m_input, errcode);
    ERRCODE_AUTOLOG(errcode);

    if (!exist)
    {
        LOG_ERROR("Input path '{}' does not exist", config_params.m_input.generic_string());
        return {};
    }

    // recursive_directory_iterator may fail on construction (e.g. permissions)
    auto it = fs::recursive_directory_iterator(config_params.m_input, errcode);
    ERRCODE_AUTOLOG(errcode);

    std::vector<fs::path> csv_files;

    for (; it != fs::recursive_directory_iterator{}; it.increment(errcode))
    {
        ERRCODE_AUTOLOG(errcode);

        // Skip non-regular files (directories, symlinks, etc.)
        if (it->is_regular_file())
        {
            fs::path file(*it);
            LOG_TRACE("Found file: '{}'", file.generic_string());

            if (validateCsvPath(config_params.m_filename_masks, file))
            {
                LOG_DEBUG("Matched CSV file: '{}'", file.generic_string());
                csv_files.push_back(std::move(file));
            }
            else
            {
                LOG_TRACE("File skipped (not matching CSV/mask): '{}'", file.generic_string());
            }
        }
    }

    LOG_INFO("CSV scan completed. Found {} matching file(s)", csv_files.size());
    return csv_files;
}

bool validateCsvPath(const std::vector<std::string>& masks, const fs::path& file)
{
    // Fast reject by extension first
    if (file.extension() != ".csv")
    {
        return false;
    }

    // If no masks provided — accept all CSV files
    if (masks.empty())
    {
        return true;
    }

    const std::string stem = file.stem().string();

    for (const auto& mask : masks)
    {
        // Check if filename contains mask substring
        // (simple substring match, not regex or glob)
        if (stem.find(mask) != std::string::npos)
        {
            LOG_TRACE("File '{}' matched mask '{}'", stem, mask);
            return true;
        }
    }

    LOG_TRACE("CSV file '{}' rejected by masks", stem);
    return false;
}

// Splits string_view into two parts by delimiter.
// First  -> before delimiter
// Second -> after delimiter (empty if delimiter not found)
std::pair<std::string_view, std::string_view> splitView(std::string_view view, char delim)
{
    size_t delim_idx = view.find(delim);

    if (delim_idx != std::string_view::npos)
    {
        return std::make_pair(
            view.substr(0, delim_idx),
            view.substr(delim_idx + 1)
        );
    }

    // Delimiter not found: return whole string as first part
    return std::make_pair(view, std::string_view());
}

bool safeGetline(std::istream& is, std::string& line)
{
    if (!std::getline(is, line))
    {
        // EOF or stream error
        LOG_TRACE("safeGetline: reached EOF or stream error");
        return false;
    }

    // Handle Windows CRLF line endings (\r\n)
    // std::getline removes '\n' but leaves '\r'
    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }

    return true;
}


// TODO: improve lifetime management (currently global pool)
// Simple string buffer pool to reduce reallocations in hot paths
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

            LOG_TRACE("Reusing string buffer (capacity={})", buff.capacity());
            return buff;
        }
    }

    // No reusable buffers available → allocate new
    std::string buff;
    buff.reserve(512);

    LOG_TRACE("Allocating new string buffer (capacity=512)");
    return buff;
}

void pushStringBuffer(std::string&& buff)
{
    std::scoped_lock lock(string_buffer_mtx);

    if (buff.capacity() > 0)
    {
        // Clear content but keep capacity for reuse
        buff.clear();

        LOG_TRACE("Returning string buffer to pool (capacity={})", buff.capacity());
        s_string_buffers.push_back(std::move(buff));
    }
    else
    {
        LOG_TRACE("Discarding empty-capacity buffer");
    }
}