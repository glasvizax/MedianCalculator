
#include <boost/program_options.hpp>
#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <iostream>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    initLogger();
    
    spdlog::warn("hello");
}

