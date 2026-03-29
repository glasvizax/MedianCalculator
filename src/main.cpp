
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
#include "Algorithms.h"

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

    /*
    
    std::queue<...> ...;

    std::condition_variable median_write_cv;

    std::thread write_csv_thread([]()
        {



        });
    */

    medianDeviationDataArray(data_array,
        [](auto element)
        {
            return std::get<1>(element);
        },
        [](auto median_tuple)
        {
            std::cout << std::fixed << std::get<1>(median_tuple) << std::endl;
        }
    );


    //write_csv_thread.join();
}


