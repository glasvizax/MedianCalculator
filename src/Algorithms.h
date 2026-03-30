#pragma once

#include <vector>
#include <tuple>
#include <algorithm>

#include "Logging.h"

template <typename... Ts>
using data_element_t = std::tuple<Ts...>;

template <typename... Ts>
using data_t = std::vector<data_element_t<Ts...>>;

template <typename... Ts>
using data_array_t = std::vector<data_t<Ts...>>;

template <typename... Ts, typename Compare>
void stableSortDataArray(data_array_t<Ts...>& data_arrays, Compare pred)
{
    LOG_DEBUG("stableSortDataArray(): begin, arrays={}", data_arrays.size());

    // Comparator:
    // - empty arrays go last
    // - otherwise compare by first element
    auto displace_empty_comp = [pred](const data_t<Ts...>& a, const data_t<Ts...>& b)
        {
            if (a.empty()) 
            {
                return false; 
            }
            if (b.empty()) 
            { 
                return true; 
            }
            return pred(a[0], b[0]);
        };

    // Step 1: sort each inner array independently
    for (auto& data : data_arrays)
    {
        std::stable_sort(data.begin(), data.end(), pred);
    }

    LOG_TRACE("Step 1 complete: inner arrays sorted");

    // Step 2: sort arrays by first element
    std::stable_sort(data_arrays.begin(), data_arrays.end(), displace_empty_comp);

    LOG_TRACE("Step 2 complete: arrays reordered");

    // Step 3: remove trailing empty arrays
    size_t removed = 0;
    while (!data_arrays.empty() && data_arrays.back().empty())
    {
        data_arrays.pop_back();
        ++removed;
    }

    if (removed)
    {
        LOG_DEBUG("Removed {} trailing empty arrays", removed);
    }

    bool has_empty = false;

    // Step 4: merge overlapping / non-strictly-ordered ranges
    for (int i = 0; i + 1 < data_arrays.size();)
    {
        int next = i + 1;

        // Find maximal range [i, next) that must be merged
        // (i.e., not strictly ordered by predicate)
        while (next < data_arrays.size())
        {
            bool stop = true;

            for (int j = i; j < next; ++j)
            {
                // If NOT strictly ordered → must merge
                if (!pred(data_arrays[j].back(), data_arrays[next].front()))
                {
                    next++;
                    stop = false;
                    break;
                }
            }

            if (stop)
                break;
        }

        size_t append_size = 0;
        for (int j = i + 1; j < next; ++j)
        {
            append_size += data_arrays[j].size();
        }

        if (append_size)
        {
            LOG_DEBUG("Merging arrays in range [{}..{})", i, next);

            data_t<Ts...> new_data;
            new_data.resize(data_arrays[i].size() + append_size);

            size_t start = 0;

            if (int j = i + 1; j < next)
            {
                // Initial merge of first two arrays
                std::merge(
                    data_arrays[i].begin(), data_arrays[i].end(),
                    data_arrays[j].begin(), data_arrays[j].end(),
                    new_data.begin(),
                    pred
                );

                start += data_arrays[i].size() + data_arrays[j].size();

                data_arrays[j].clear();
                has_empty = true;
                j++;

                // Merge remaining arrays incrementally
                for (; j < next; ++j)
                {
                    // Fast path for trivially copyable tuples
                    if constexpr ((std::is_trivially_copyable_v<Ts> && ...))
                    {
                        std::memcpy(
                            new_data.data() + start,
                            data_arrays[j].data(),
                            data_arrays[j].size() * sizeof(std::tuple<Ts...>)
                        );
                    }
                    else
                    {
                        std::copy(
                            data_arrays[j].begin(),
                            data_arrays[j].end(),
                            new_data.begin() + start
                        );
                    }

                    // Merge appended chunk into prefix
                    std::inplace_merge(
                        new_data.begin(),
                        new_data.begin() + start,
                        new_data.begin() + start + data_arrays[j].size(),
                        pred
                    );

                    start += data_arrays[j].size();
                    data_arrays[j].clear();
                }
            }

            data_arrays[i] = std::move(new_data);
        }

        i = next;
    }

    // Step 5: cleanup empty arrays if any were created
    if (has_empty)
    {
        LOG_DEBUG("Cleaning up empty arrays after merge");

        std::stable_sort(data_arrays.begin(), data_arrays.end(),
            [pred](auto& a, auto& b)
            {
                if (a.empty()) return false;
                if (b.empty()) return true;
                return pred(a[0], b[0]);
            });

        size_t removed2 = 0;
        while (!data_arrays.empty() && data_arrays.back().empty())
        {
            data_arrays.pop_back();
            ++removed2;
        }

        LOG_DEBUG("Removed {} empty arrays after merge", removed2);
    }

    LOG_DEBUG("stableSortDataArray(): done, arrays={}", data_arrays.size());
}

// data_array must be sorted
template <typename... Ts, typename ElementGetter, typename OnMedianDeviated>
void medianDeviationDataArray(
    const data_array_t<Ts...>& data_array,
    ElementGetter element_getter,
    OnMedianDeviated on_median_deviated,
    long double accuracy = 1e-8)
{
    LOG_DEBUG("medianDeviationDataArray(): begin, arrays={}", data_array.size());

    long double prev_median = 0.0;
    long double median = 0.0;

    bool even = true;

    // Find first non-empty array
    auto array_it = data_array.begin();
    if (array_it == data_array.end())
    {
        LOG_DEBUG("Empty data_array");
        return;
    }

    auto data_it = array_it->begin();

    // Skip empty arrays
    while (data_it == array_it->end())
    {
        array_it++;
        if (array_it == data_array.end())
        {
            LOG_DEBUG("All arrays are empty");
            return;
        }
        data_it = array_it->begin();
    }

    // Active cursor (global iterator across all arrays)
    auto ac_array_it = array_it;
    auto ac_data_it = data_it;

    auto _t = element_getter(*data_it);
    decltype(_t) _k;

    median = _t;

    // Emit first median deviation
    if (std::abs(median - prev_median) >= accuracy)
    {
        on_median_deviated(*ac_data_it);
    }
    prev_median = median;

    even = !even;
    data_it++;

    // Advance global iterator
    ac_data_it++;
    while (ac_data_it == ac_array_it->end())
    {
        ac_array_it++;
        if (ac_array_it == data_array.end())
        {
            return;
        }
        ac_data_it = ac_array_it->begin();
    }

    // Main processing loop
    for (; data_it != array_it->end();)
    {
        if (even)
        {
            // Odd count → median is previous element
            _t = _k;
            median = _t;

            if (std::abs(median - prev_median) >= accuracy)
            {
                on_median_deviated(*ac_data_it);
            }

            prev_median = median;

            _k = element_getter(*data_it);
            ++data_it;
        }
        else
        {
            // Even count → average of two elements
            _k = element_getter(*data_it);
            median = (_k + _t) / 2.0;

            if (std::abs(median - prev_median) >= accuracy)
            {
                on_median_deviated(*ac_data_it);
            }

            prev_median = median;
        }

        // Advance global iterator across arrays
        ac_data_it++;
        while (ac_data_it == ac_array_it->end())
        {
            ac_array_it++;
            if (ac_array_it == data_array.end())
            {
                return;
            }
            ac_data_it = ac_array_it->begin();
        }

        even = !even;
    }

    // Continue for remaining arrays
    array_it++;
    for (; array_it != data_array.end(); ++array_it)
    {
        for (data_it = array_it->begin(); data_it != array_it->end();)
        {
            if (even)
            {
                _t = _k;
                median = _t;

                if (std::abs(median - prev_median) >= accuracy)
                {
                    on_median_deviated(*ac_data_it);
                }

                prev_median = median;
                ++data_it;
            }
            else
            {
                _k = element_getter(*data_it);
                median = (_k + _t) / 2.0;

                if (std::abs(median - prev_median) >= accuracy)
                {
                    on_median_deviated(*ac_data_it);
                }

                prev_median = median;
            }

            ac_data_it++;
            while (ac_data_it == ac_array_it->end())
            {
                ac_array_it++;
                if (ac_array_it == data_array.end())
                {
                    return;
                }
                ac_data_it = ac_array_it->begin();
            }

            even = !even;
        }
    }

    LOG_DEBUG("medianDeviationDataArray(): done");
}
