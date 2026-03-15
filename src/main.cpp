
#include <boost/program_options.hpp>
#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>

#include "Logging.h"

namespace po = boost::program_options;
using namespace std::string_view_literals;

int main(int argc, char** argv)
{
    initDefaultLoggers();

    LOG_CRITICAL("error 2");
}

