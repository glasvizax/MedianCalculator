#pragma once

#include <vector>
#include <tuple>
#include <algorithm>

template <typename... Ts>
using data_t = std::vector<std::tuple<Ts...>>;

template <typename... Ts>
using data_array_t = std::vector<data_t<Ts...>>;


// pred MUST be strict 
template <typename... Ts, typename Compare>
void stableSortDataArray(data_array_t<Ts...>& data_arrays, Compare pred)
{
    // Comparator that pushes empty arrays to the end
    // and compares arrays by their first element
    auto displace_empty_comp = [pred](const data_t<Ts...>& a, const data_t<Ts...>& b)
        {
            if (a.empty()) return false;
            if (b.empty()) return true;
            return pred(a[0], b[0]);
        };

    // Step 1: sort each inner array independently
    for (auto& data : data_arrays)
    {
        std::stable_sort(data.begin(), data.end(), pred);
    }

    // Step 2: sort arrays by their first element (empty ones go last)
    std::stable_sort(data_arrays.begin(), data_arrays.end(), displace_empty_comp);

    // Step 3: remove trailing empty arrays
    while (!data_arrays.empty() && data_arrays.back().empty())
    {
        data_arrays.pop_back();
    }

    bool has_empty = false;

    // Step 4: try to merge consecutive compatible ranges
    for (int i = 0; i + 1 < data_arrays.size();)
    {
        int next = i + 1;

        // Find maximal range [i, next) that cannot be strictly separated
        // i.e., ranges overlap or are not strictly ordered
        while (next < data_arrays.size())
        {
            bool stop = true;

            // Check if next array can be placed after ALL arrays in [i, next)
            for (int j = i; j < next; ++j)
            {
                // If NOT strictly ordered, we must merge
                if (!pred(data_arrays[j].back(), data_arrays[next].front()))
                {
                    next++;     // extend merge range
                    stop = false;
                    break;
                }
            }

            if (stop)
            {
                break;
            }
        }

        // Calculate total size of arrays to append
        size_t _append = 0;
        for (int j = i + 1; j < next; ++j)
        {
            _append += data_arrays[j].size();
        }

        // If there is something to merge
        if (_append)
        {
            data_t<Ts...> new_data;
            new_data.resize(data_arrays[i].size() + _append);

            size_t start = 0;

            // First merge: data_arrays[i] + data_arrays[i+1]
            if (int j = i + 1; j < next)
            {
                std::merge(
                    data_arrays[i].begin(), data_arrays[i].end(),
                    data_arrays[j].begin(), data_arrays[j].end(),
                    new_data.begin(),
                    pred
                );

                start += data_arrays[i].size() + data_arrays[j].size();

                data_arrays[j].clear(); // mark as empty after merge
                has_empty = true;
                j++;

                // Merge remaining arrays one by one
                for (; j < next; ++j)
                {
                    // Fast path for trivially copyable types
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

                    // Merge appended chunk into already merged prefix
                    std::inplace_merge(
                        new_data.begin(),
                        new_data.begin() + start,
                        new_data.begin() + start + data_arrays[j].size(),
                        pred
                    );

                    start += data_arrays[j].size();
                    data_arrays[j].clear(); // mark consumed
                }
            }

            // Replace original array with merged result
            data_arrays[i] = std::move(new_data);
        }

        // Move to next independent segment
        i = next;
    }

    // Step 5: cleanup if we created empty arrays during merging
    if (has_empty)
    {
        // Reorder to push empty arrays to the end again
        std::stable_sort(data_arrays.begin(), data_arrays.end(), [pred](auto& a, auto& b)
            {
                if (a.empty()) return false;
                if (b.empty()) return true;
                return pred(a[0], b[0]);
            });

        // Remove all trailing empty arrays
        while (!data_arrays.empty() && data_arrays.back().empty())
        {
            data_arrays.pop_back();
        }
    }
}
