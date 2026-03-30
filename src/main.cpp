
#include <boost/program_options.hpp>
#include <toml++/toml.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>
#include <thread>

#include "Logging.h"
#include "Config.h"
#include "Misc.h"
#include "CsvSetParser.h"
#include "Algorithms.h"

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    initDefaultLoggers();
    
    ConfigParams config_params = receiveConfigParams(argc, argv);
    auto csv_files = findMatchingCsvFiles(config_params);

    std::sort(csv_files.begin(), csv_files.end());

    CsvSetParser<unsigned long long, double> parser({ "receive_ts", "price" });
    auto data_array = parser.processCsvFiles(csv_files);

    stableSortDataArray(data_array,
        [](auto a, auto b)
        {
            return std::get<0>(a) < std::get<0>(b);
        }
    );
    
    std::atomic<bool> proceed(true);
    std::queue<std::tuple<unsigned long long, double>> median_elements;
    std::mutex median_elements_mtx;
    std::condition_variable median_write_cv;

    fs::create_directories(config_params.m_output.parent_path());

    std::ofstream output_file(config_params.m_output, std::ios_base::trunc);
    if(!output_file.is_open())
    {
        LOG_ERROR("some error");
        return EXIT_FAILURE;
    }
    output_file << "receive_ts" << ";" << "price" << std::endl;

    std::thread write_csv_thread([&]()
        {
            char buff1[256];
            char buff2[256];
            while (true) 
            {
                std::tuple<unsigned long long, double> current_tuple;
                {
                    std::unique_lock lock(median_elements_mtx);
                    median_write_cv.wait(lock,[&]() 
                    { 
                        return !median_elements.empty() || !proceed.load();
                    });
                    if (!proceed.load() && median_elements.empty())
                    {
                        break;
                    }
                    current_tuple = median_elements.front();
                    median_elements.pop();
                }
                if (output_file.good()) 
                {
                    auto res1 = std::to_chars(buff1, buff1 + 256, std::get<0>(current_tuple));
                    auto res2 = std::to_chars(buff2, buff2 + 256, std::get<1>(current_tuple), std::chars_format::fixed, 8);

                    *res1.ptr = '\0';
                    *res2.ptr = '\0';
                    output_file << buff1 << ";" << buff2 << std::endl;
                }
            }
        });
    
    medianDeviationDataArray(data_array,
        [](auto element)
        {
            return std::get<1>(element);
        },
        [&](auto element)
        {
            {
                std::scoped_lock lock(median_elements_mtx);
                median_elements.push(element);
            }
            median_write_cv.notify_one();
        }
    );
    proceed.store(false);
    median_write_cv.notify_all();
    write_csv_thread.join();
}


