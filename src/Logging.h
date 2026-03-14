#pragma once

#include <spdlog/spdlog.h>

using LoggerPtr = std::shared_ptr<spdlog::logger>;
using SinkPtr = spdlog::sink_ptr;

//TODO : pick the right allocator class
template<typename T>
using Allocator = std::allocator<T>;

using LoggersAllocator = Allocator<LoggerPtr>;
using SinksAllocator = Allocator<SinkPtr>;

namespace {
    LoggersAllocator g_loggers_allocator;
    SinksAllocator g_sinks_allocator;
}

LoggerPtr addLogger(const std::string& name);

template<typename SinkType>
SinkPtr addSink()
{
    SinkPtr sink = std::allocate_shared<SinkType>(g_sinks_allocator);
    return sink;
}

void initDefaultLogger();

class LoggerGuard
{
private:
    LoggerPtr m_prev_logger;
public:
    explicit LoggerGuard(LoggerPtr new_logger);
    LoggerGuard(const LoggerGuard&) = delete;
    LoggerGuard(LoggerGuard&&) = delete;
    void operator=(LoggerGuard&&) = delete;
    void operator=(const LoggerGuard&) = delete;

    ~LoggerGuard() noexcept;
};

