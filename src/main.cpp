
#include <boost/program_options.hpp>
#include <toml++/toml.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>

#include "Logging.h"
#include "Config.h"
#include "Misc.h"

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    initDefaultLoggers();
    
    ConfigParams config_params = receiveConfigParams(argc, argv);
    auto res = findMatchingCsvFiles(std::move(config_params));

    LOG_INFO("{}", res.size());
}