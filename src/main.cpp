
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
    
    CsvSetParser<std::string, double> parser({"side", "quantity"});
    auto data_arrays = parser.processCsvFiles(csv_files);

    for(auto& data : data_arrays)
    {
        for (auto [r,p] : data)
        {
            std::cout << r << "  " << p << std::endl;
        }
    }

}