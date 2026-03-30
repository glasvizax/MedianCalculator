#include <boost/program_options.hpp>
#include <toml++/toml.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>
#include <thread>
#include <queue>

#include "Logging.h"
#include "Config.h"
#include "Misc.h"
#include "CsvSetParser.h"
#include "Algorithms.h"

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    initDefaultLoggers();
    LOG_INFO("Application started");

    // Load and normalize configuration
    ConfigParams config_params = receiveConfigParams(argc, argv);

    // Discover input CSV files
    auto csv_files = findMatchingCsvFiles(config_params);
    LOG_INFO("Discovered {} CSV file(s)", csv_files.size());

    // Sort file list to ensure deterministic processing order
    std::sort(csv_files.begin(), csv_files.end());
    LOG_DEBUG("CSV files sorted");

    // Parse CSV files into data arrays
    CsvSetParser<unsigned long long, double> parser({ "receive_ts", "price" });
    LOG_INFO("Parsing CSV files...");
    auto data_array = parser.processCsvFiles(csv_files);
    LOG_INFO("CSV parsing completed. Arrays count = {}", data_array.size());

    // Sort and merge data arrays by timestamp (first tuple element)
    LOG_INFO("Sorting and merging data arrays...");
    stableSortDataArray(data_array,
        [](auto a, auto b)
        {
            return std::get<0>(a) < std::get<0>(b);
        }
    );
    LOG_INFO("Data arrays sorted and merged");

    // Shared state for producer-consumer pipeline:
    // - main thread produces median elements
    // - writer thread consumes and writes to file
    std::atomic<bool> proceed(true);
    std::queue<std::tuple<unsigned long long, double>> median_elements;
    std::mutex median_elements_mtx;
    std::condition_variable median_write_cv;

    // Ensure output directory exists
    fs::create_directories(config_params.m_output.parent_path());
    LOG_DEBUG("Ensured output directory exists: '{}'",
        config_params.m_output.parent_path().generic_string());

    // Open output file
    std::ofstream output_file(config_params.m_output, std::ios_base::trunc);
    if (!output_file.is_open())
    {
        LOG_ERROR("Failed to open output file '{}'", config_params.m_output.generic_string());
        return EXIT_FAILURE;
    }

    LOG_INFO("Writing results to '{}'", config_params.m_output.generic_string());

    // Write CSV header
    output_file << "receive_ts" << ";" << "price" << std::endl;

    // Writer thread:
    // waits for new median elements and writes them to file
    std::thread write_csv_thread([&]()
        {
            LOG_DEBUG("Writer thread started");

            char buff1[256];
            char buff2[256];

            while (true)
            {
                std::tuple<unsigned long long, double> current_tuple;

                {
                    std::unique_lock lock(median_elements_mtx);

                    // Wait until:
                    // - new data is available OR
                    // - processing is finished
                    median_write_cv.wait(lock, [&]()
                        {
                            return !median_elements.empty() || !proceed.load();
                        });

                    // Exit condition:
                    // no more data will arrive AND queue is empty
                    if (!proceed.load() && median_elements.empty())
                    {
                        LOG_DEBUG("Writer thread exiting (no more data)");
                        break;
                    }

                    current_tuple = median_elements.front();
                    median_elements.pop();
                }

                if (output_file.good())
                {
                    // Fast numeric formatting using std::to_chars
                    auto res1 = std::to_chars(buff1, buff1 + 256, std::get<0>(current_tuple));
                    auto res2 = std::to_chars(buff2, buff2 + 256, std::get<1>(current_tuple),
                        std::chars_format::fixed, 8);

                    *res1.ptr = '\0';
                    *res2.ptr = '\0';

                    output_file << buff1 << ";" << buff2 << std::endl;
                }
                else
                {
                    LOG_ERROR("Output file stream is in bad state");
                }
            }

            LOG_DEBUG("Writer thread finished");
        });

    LOG_INFO("Starting median calculation");

    // Process data and emit elements where median deviates
    medianDeviationDataArray(
        data_array,
        [](auto element)
        {
            return std::get<1>(element); // extract price
        },
        [&](auto element)
        {
            {
                std::scoped_lock lock(median_elements_mtx);
                median_elements.push(element);
            }

            // Notify writer thread that new data is available
            median_write_cv.notify_one();
        }
    );

    LOG_INFO("Median calculation completed");

    // Signal writer thread to finish
    proceed.store(false);
    median_write_cv.notify_all();

    LOG_DEBUG("Waiting for writer thread to join...");
    write_csv_thread.join();

    LOG_INFO("Application finished successfully");
}