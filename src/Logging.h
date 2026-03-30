#pragma once

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> g_stdout_logger;
extern std::shared_ptr<spdlog::logger> g_stderr_logger;

void initDefaultLoggers(spdlog::level::level_enum log_level = spdlog::level::level_enum::info);

#ifdef _DEBUG
	#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(g_stdout_logger, __VA_ARGS__)
	#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(g_stdout_logger, __VA_ARGS__)
	#define LOG_INFO(...) SPDLOG_LOGGER_INFO(g_stdout_logger, __VA_ARGS__)
	#define LOG_WARN(...) SPDLOG_LOGGER_WARN(g_stderr_logger, __VA_ARGS__)
	#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(g_stderr_logger, __VA_ARGS__)
	#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(g_stderr_logger, __VA_ARGS__)
#else 
	#define LOG_TRACE(...) do {} while (0)
	#define LOG_DEBUG(...) do {} while (0)
	#define LOG_INFO(...) do {} while (0)
	#define LOG_WARN(...) do {} while (0)
	#define LOG_ERROR(...) do {} while (0)
	#define LOG_CRITICAL(...) do {} while (0)
#endif

#define ERRCODE_AUTOLOG(var) do {if(var) { LOG_CRITICAL(var.message()); std::exit(EXIT_FAILURE); }} while(0)