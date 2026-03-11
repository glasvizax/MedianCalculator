cmake_minimum_required(VERSION 3.23)

cmake_policy(SET CMP0167 NEW)
project(MedianCalculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(Boost_DIR "" CACHE PATH "path to Boost")

find_package(Boost CONFIG REQUIRED COMPONENTS program_options)

add_executable(Main src/main.cpp)

target_link_libraries(Main PRIVATE Boost::program_options)
