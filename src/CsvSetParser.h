
#include <optional>
#include <unordered_set>
#include <execution>

#include "Misc.h"
#include "Logging.h"

template<typename... Ts>
class CsvSetParser
{
    inline static constexpr std::size_t s_column_count = sizeof...(Ts);

    template <std::size_t ColumnCount>
    struct header_info
    {
        // first elem - column index, second - tuple index
        std::array<std::pair<unsigned int, unsigned int>, ColumnCount> indices;
        unsigned int latest_idx;
        unsigned int size_in_bytes;
    };

    using column_t = std::tuple<Ts...>;
    using block_t = std::vector<column_t>;
    using block_array_t = std::vector<block_t>;
    using header_info_t = header_info<s_column_count>;

public:

    CsvSetParser(std::array<std::string, s_column_count> names) : m_names(names) {}

    CsvSetParser(const CsvSetParser&) = delete;
    CsvSetParser& operator=(const CsvSetParser&) = delete;
    CsvSetParser(CsvSetParser&&) = delete;
    CsvSetParser& operator=(CsvSetParser&&) = delete;

    block_array_t processCsvFiles(const std::vector<fs::path>& csv_files);

    std::optional<block_t> parseSingleStream(std::istream& in, uintmax_t stream_size, std::string_view stream_name = "memory_stream");

private:

    std::optional<block_t> collectCsvData(std::istream& in, uintmax_t stream_size, std::string_view stream_name, header_info_t& header_info);

    std::optional<column_t> parseCsvRow(std::string_view row_str, header_info_t& header_info);

    std::optional<header_info_t> parseCsvHeaders(std::istream& in);

    template<size_t I = 0>
    bool parseCsvCell(std::size_t idx, std::string_view before, column_t& tuple);

private:

    const std::array<std::string, s_column_count> m_names;
    std::map<size_t, block_t> m_free_blocks;
    std::mutex m_free_blocks_mtx;
};

template<typename ...Ts>
inline typename CsvSetParser<Ts...>::block_array_t CsvSetParser<Ts...>::processCsvFiles(const std::vector<fs::path>& csv_files)
{
    block_array_t data_arrays;
    std::mutex data_arrays_mtx;
    data_arrays.resize(csv_files.size());

    std::for_each(std::execution::par, csv_files.begin(), csv_files.end(), 
        [this, &data_arrays, &data_arrays_mtx, &csv_files] (auto& path)
        {
            size_t current = &path - csv_files.data();

            LOG_INFO("Processing CSV file: {}", path.generic_string());

            std::ifstream file(path, std::ios_base::binary);
            if (!file)
            {
                LOG_WARN("Couldn't open file [{}], skipping it", path.generic_string());
                return;
            }

            auto data_array = parseSingleStream(file, fs::file_size(path), path.generic_string());
            if (!data_array)
            {
                return;
            }

            if (!data_array->empty())
            {
                std::size_t sz;
                {
                    std::scoped_lock lock(data_arrays_mtx);
                    data_arrays[current] = std::move(*data_array);
                    std::size_t sz = data_arrays.back().size();
                }
                LOG_TRACE("Added data array of size {} for file [{}]", sz, path.generic_string());
            }
        });

    LOG_INFO("Finished processing {} CSV files", data_arrays.size());
    return data_arrays;
}

template<typename ...Ts>
inline std::optional<typename CsvSetParser<Ts...>::block_t> CsvSetParser<Ts...>::parseSingleStream(std::istream& in, uintmax_t stream_size, std::string_view stream_name)
{
    auto header_info = parseCsvHeaders(in);
    if (!header_info)
    {
        LOG_WARN("Incorrect CSV headers in file [{}], skipping", stream_name);
        return std::nullopt;
    }

    return collectCsvData(in, stream_size, stream_name, *header_info);
}

template<typename ...Ts>
inline std::optional<typename CsvSetParser<Ts...>::block_t> CsvSetParser<Ts...>::collectCsvData(std::istream& in, uintmax_t stream_size, std::string_view stream_name, header_info_t& header_info)
{
    unsigned int row_size;
    unsigned int curr_idx = 2; // current line index (after the header)
    StringLease row;
    block_t data_array;

    if (!safeGetline(in, row.str()))
    {
        LOG_ERROR("Received empty stream for file [{}]", stream_name);
        return data_array;
    }

    // calculating the approximate block size for pre-allocating memory
    // TODO : improve algorithm 
    row_size = static_cast<unsigned int>(in.tellg()) - header_info.size_in_bytes;
    unsigned int approx_cap = ((stream_size - header_info.size_in_bytes) / row_size) + 1;

    LOG_DEBUG("Reserving approx {} rows for file [{}]", approx_cap, stream_name);

    // use the free block cache if available.
    bool _log = false;
    {
        std::scoped_lock lock(m_free_blocks_mtx);
        if (!m_free_blocks.empty())
        {
            auto lb = m_free_blocks.lower_bound(approx_cap);
            if (lb != m_free_blocks.end())
            {
                data_array = std::move(lb->second);
                m_free_blocks.erase(lb);
                _log = true;
            }
        }
    }

    if(_log)
    {
        LOG_TRACE("Reusing cached block with capacity {} for file [{}]", approx_cap, stream_name);
    }

    data_array.reserve(approx_cap);

    // parse the first line after the header
    {
        auto res = parseCsvRow(row.str(), header_info);
        if (!res)
        {
            LOG_WARN("Invalid data in row {} of file [{}], skipping file", curr_idx, stream_name);

            if (data_array.capacity() > 0)
            {
                // save to cache for future use.
                size_t cap = data_array.capacity();
                std::scoped_lock lock(m_free_blocks_mtx);
                m_free_blocks.emplace(cap, std::move(data_array));
            }

            return std::nullopt;
        }
        data_array.emplace_back(*res);
    }

    // parse remaining lines
    while (safeGetline(in, row.str()))
    {
        auto res = parseCsvRow(row.str(), header_info);
        if (!res)
        {
            LOG_WARN("Invalid data in row {} of file [{}], skipping file", curr_idx, stream_name);

            if (data_array.capacity() > 0)
            {
                // save to cache for future use.
                size_t cap = data_array.capacity();
                std::scoped_lock lock(m_free_blocks_mtx);
                m_free_blocks.emplace(cap, std::move(data_array));
            }

            return std::nullopt;
        }

        data_array.emplace_back(*res);
        curr_idx++;
    }

    LOG_INFO("Collected {} rows from file [{}]", data_array.size(), stream_name);
    return data_array;
}


template<typename ...Ts>
inline std::optional<typename CsvSetParser<Ts...>::column_t> CsvSetParser<Ts...>::parseCsvRow(std::string_view row_str, header_info_t& header_info)
{
    int curr_idx = 0;
    column_t column;

    std::string_view after(row_str);

    // We go through all the column indices
    for (auto [c, i] : header_info.indices)
    {
        std::string_view before;
        while (c >= curr_idx)
        {
            if (after.empty())
            {
                LOG_WARN("Row ended prematurely while parsing column index {}", c);
                return std::nullopt;
            }
            std::tie(before, after) = splitView(after);
            curr_idx++;
        }
        if (!parseCsvCell(i, before, column))
        {
            LOG_WARN("Failed to parse cell at tuple index {} (CSV column {})", i, c);
            return std::nullopt;
        }
    }

    return column;
}

template<typename ...Ts>
inline std::optional<typename CsvSetParser<Ts...>::header_info_t> CsvSetParser<Ts...>::parseCsvHeaders(std::istream& in)
{
    StringLease line;
    if (!safeGetline(in, line.str()))
    {
        LOG_ERROR("Received empty stream while reading headers");
        return std::nullopt;
    }

    unsigned int curr_idx = 0;
    header_info_t header_info;
    header_info.indices.fill(std::make_pair(-1, -1));

    std::string_view after(line.str());

    while (!after.empty())
    {
        std::string_view before;
        std::tie(before, after) = splitView(after);

        for (unsigned int i = 0u; i < m_names.size(); ++i)
        {
            if (before == m_names[i])
            {
                header_info.indices[i] = std::make_pair(curr_idx, i);
            }
        }
        ++curr_idx;
    }

    for (unsigned int i = 0u; i < header_info.indices.size(); ++i)
    {
        if (header_info.indices[i].first == static_cast<unsigned int>(-1))
        {
            LOG_WARN("Can't find required column [{}] in CSV", m_names[i]);
            return std::nullopt;
        }
    }

    // Sorting by column index in CSV for correct reading of rows
    std::sort(header_info.indices.begin(), header_info.indices.end(),
        [](std::pair<unsigned int, unsigned int> a, std::pair<unsigned int, unsigned int> b)
        {
            return a.first < b.first;
        }
    );

    header_info.latest_idx = curr_idx;
    header_info.size_in_bytes = in.tellg();

    LOG_DEBUG("Parsed CSV headers, found {} columns, stream position {}", curr_idx, header_info.size_in_bytes);
    return header_info;
}


template<typename ...Ts>
template<size_t I>
inline bool CsvSetParser<Ts...>::parseCsvCell(std::size_t idx, std::string_view before, column_t& tuple)
{
    if constexpr (I < std::tuple_size_v<column_t>)
    {
        if (idx > I)
        {
            // recursively go to the desired element
            return parseCsvCell<I + 1>(idx, before, tuple);
        }

        using ElementType = std::tuple_element_t<I, column_t>;

        if constexpr (std::is_arithmetic_v<ElementType>)
        {
            auto [ptr, ec] = std::from_chars(before.data(), before.data() + before.size(), std::get<I>(tuple));
            if (ec != std::errc{})
            {
                std::error_code errcode = std::make_error_code(ec);
                LOG_WARN("Error parsing numeric cell at tuple index {}: {}", I, errcode.message());
                return false;
            }
            if (ptr != before.data() + before.size())
            {
                LOG_WARN("Extra characters found in numeric cell at tuple index {}", I);
                return false;
            }
            return true;
        }
        else
        {
            std::get<I>(tuple) = ElementType(before);
            return true;
        }
    }

    LOG_WARN("Column index {} out of tuple bounds", idx);
    return false;
}

