cmake_minimum_required(VERSION 3.23)

include(FetchContent)
cmake_policy(SET CMP0167 NEW)
project(MedianCalculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(Boost_DIR "" CACHE PATH "path to Boost config file")

find_package(Boost CONFIG REQUIRED COMPONENTS program_options)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_PROGRESS TRUE
    SYSTEM
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.17.0
    GIT_PROGRESS TRUE
    SYSTEM
)

FetchContent_MakeAvailable(tomlplusplus spdlog)

add_executable(Main src/main.cpp)

target_link_libraries(Main PRIVATE Boost::program_options spdlog::spdlog tomlplusplus::tomlplusplus)


