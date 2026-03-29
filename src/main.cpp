
#include <boost/program_options.hpp>
#include <toml++/toml.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>

#include "Logging.h"
#include "Config.h"
#include "Misc.h"
#include "CsvSetParser.h"

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    initDefaultLoggers();

    ConfigParams config_params = receiveConfigParams(argc, argv);
    auto csv_files = findMatchingCsvFiles(std::move(config_params));

    std::sort(csv_files.begin(), csv_files.end());

    CsvSetParser<unsigned long long, double> parser({ "receive_ts", "price" });
    auto data_array = parser.processCsvFiles(csv_files);

    stableSortDataArray(data_array, 
        [](auto a, auto b) 
        { 
            return std::get<0>(a) < std::get<0>(b); 
        }
    );

    for (auto& arr : data_array)
    {
        for (auto [r,p] : arr)
        {
            std::cout << r << " " << p << std::endl;
        }
        std::cout << std::endl;
    }
}


