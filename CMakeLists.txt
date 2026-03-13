cmake_minimum_required(VERSION 3.23)

include(FetchContent)
cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0135 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)

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
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.17.0
    GIT_PROGRESS TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(
    tomlplusplus 
    spdlog 
)

set(_mc_sources 
    src/Logging.h 
    src/Logging.cpp
)

set(USE_PCH FALSE CACHE BOOL "turn on to use pch")

add_library(Core STATIC ${_mc_sources} $<$<BOOL:USE_PCH>:src/default_pch.h>)

target_link_libraries(Core 
    PUBLIC 
    Boost::program_options 
    spdlog::spdlog 
    tomlplusplus::tomlplusplus
)

target_include_directories(Core PUBLIC src)

target_precompile_headers(Core 
    PUBLIC 
    $<$<BOOL:USE_PCH>:"default_pch.h">
)

add_executable(Main 
    src/main.cpp 
)

target_link_libraries(Main 
    PRIVATE 
    Core
)

target_precompile_headers(Main REUSE_FROM Core)

set(TEST_MedianCalculator TRUE CACHE BOOL "turn off to drop tests")

if(TEST_MedianCalculator AND PROJECT_IS_TOP_LEVEL)
    enable_testing()
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        SYSTEM
        EXCLUDE_FROM_ALL
    )
    
    FetchContent_MakeAvailable(
        googletest
    )
    
    add_executable(Test test/test1.cpp)

    target_include_directories(Test PRIVATE test)

    target_link_libraries(Test PRIVATE 
        GTest::gtest_main 
        GTest::gtest
        Core
    )

    include(GoogleTest)
    gtest_discover_tests(
        Test
        DISCOVERY_MODE PRE_TEST
    )

    target_precompile_headers(Test 
        REUSE_FROM 
        Core
    )

endif()