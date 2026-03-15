#pragma once

#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> g_stdout_logger;
extern std::shared_ptr<spdlog::logger> g_stderr_logger;

void initDefaultLoggers();

#define LOG_TRACE(_msg_) SPDLOG_LOGGER_TRACE(g_stdout_logger, _msg_);
#define LOG_DEBUG(_msg_) SPDLOG_LOGGER_DEBUG(g_stdout_logger, _msg_);
#define LOG_INFO(_msg_) SPDLOG_LOGGER_INFO(g_stdout_logger, _msg_);
#define LOG_WARN(_msg_) SPDLOG_LOGGER_WARN(g_stderr_logger, _msg_);
#define LOG_ERROR(_msg_) SPDLOG_LOGGER_ERROR(g_stderr_logger, _msg_);
#define LOG_CRITICAL(_msg_) SPDLOG_LOGGER_CRITICAL(g_stderr_logger, _msg_);

