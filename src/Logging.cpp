#include "Logging.h"

LoggerPtr addLogger(const std::string& name)
{
    LoggerPtr logger = std::allocate_shared<LoggerPtr::element_type>(g_loggers_allocator, name);
    return logger;
}

void initDefaultLogger()
{
    LoggerPtr default_logger = addLogger("default logger");
    SinkPtr default_sink = addSink<spdlog::sinks::stdout_color_sink_mt>();

    default_logger->sinks().push_back(default_sink);
    spdlog::set_default_logger(default_logger);
}

LoggerGuard::LoggerGuard(LoggerPtr new_logger)
{
    if (new_logger)
    {
        m_prev_logger = spdlog::default_logger();
        spdlog::set_default_logger(new_logger);
    }
    else
    {
        spdlog::error("new_loger is null pointer");
    }
}

LoggerGuard::~LoggerGuard() noexcept
{
    spdlog::set_default_logger(m_prev_logger);
}
