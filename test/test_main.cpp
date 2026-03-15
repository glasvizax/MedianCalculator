#include <gtest/gtest.h>

#include <spdlog/spdlog.h>

#include "Logging.h"

constexpr const char* TEST_LOG_PATTERN = "\n\t[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v\n";

void setupLogging();

int main(int argc, char** argv) 
{
	testing::InitGoogleTest(&argc, argv);

	setupLogging();

	return RUN_ALL_TESTS();
}

void setupLogging()
{
	initDefaultLoggers();

#ifdef DROP_LOGGING
	spdlog::set_level(spdlog::level::off);
#else
	spdlog::set_pattern(TEST_LOG_PATTERN);
#endif
}