#include "Logging.h"

constexpr const char* LOG_PATTERN = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] [%s:%#] %v";

#ifdef _DEBUG
std::shared_ptr<spdlog::logger> g_stdout_logger;
std::shared_ptr<spdlog::logger> g_stderr_logger;
#endif

void initDefaultLoggers(spdlog::level::level_enum log_level)
{
#ifdef _DEBUG
	g_stdout_logger = spdlog::stdout_color_mt("stdout_logger");
	g_stderr_logger = spdlog::stderr_color_mt("stderr_logger");

	g_stdout_logger->set_pattern(LOG_PATTERN);
	g_stderr_logger->set_pattern(LOG_PATTERN);

	g_stdout_logger->set_level(log_level);
	g_stderr_logger->set_level(log_level);
#endif
}
