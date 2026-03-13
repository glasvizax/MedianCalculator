#include <gtest/gtest.h>

#include <boost/program_options.hpp>
#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "Logging.h"

TEST(LoggingTests, LoggerGuardTest) 
{	
	auto default_logger = spdlog::default_logger();

	auto custom_logger = addLogger("custom logger");
	{
		LoggerGuard lg(custom_logger);

		EXPECT_EQ(spdlog::default_logger(), custom_logger);
	}

	EXPECT_EQ(spdlog::default_logger(), default_logger);
}