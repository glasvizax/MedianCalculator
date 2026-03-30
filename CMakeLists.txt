cmake_minimum_required(VERSION 3.23)

include(FetchContent)

# Enable modern behavior for dependency handling and FetchContent
cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0135 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)

project(MedianCalculator VERSION 1.0.0 LANGUAGES CXX)

message(STATUS "Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")

# --- C++ standard ---

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# --- Output directories ---

set(DEBUG_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug)
set(RELEASE_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/Release)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEBUG_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RELEASE_BIN_DIR})

message(STATUS "Debug output dir: ${DEBUG_BIN_DIR}")
message(STATUS "Release output dir: ${RELEASE_BIN_DIR}")

# --- Boost configuration (manual discovery) ---
# NOTE: We force user to provide Boost_ROOT instead of relying on system paths

set(Boost_ROOT "" CACHE PATH "path to Boost root dir")

if(NOT Boost_ROOT)
    message(FATAL_ERROR "Boost_ROOT must be set (e.g. -DBoost_ROOT=/path/to/boost)")
endif()

message(STATUS "Boost root: ${Boost_ROOT}")

# Try to locate BoostConfig.cmake manually
list(APPEND _boost_config_paths "${Boost_ROOT}/lib*/cmake/Boost*/BoostConfig.cmake")
list(APPEND _boost_config_paths "${Boost_ROOT}/lib*/cmake/Boost*/boost-config.cmake")
list(APPEND _boost_config_paths "${Boost_ROOT}/lib*/cmake/Boost*/Boost.cps")
list(APPEND _boost_config_paths "${Boost_ROOT}/lib*/cmake/Boost*/boost.cps")

file(GLOB _boost_configs ${_boost_config_paths})

foreach(_config IN LISTS _boost_configs)
    message(STATUS "Checking Boost config candidate: ${_config}")
    if(EXISTS ${_config})
        set(BOOST_CONFIG_FILE ${_config})
        break()
    endif()
endforeach()

if(NOT BOOST_CONFIG_FILE)
    message(FATAL_ERROR "Failed to locate Boost config file under Boost_ROOT")
endif()

cmake_path(GET BOOST_CONFIG_FILE PARENT_PATH BOOST_CONFIG_PATH)

set(Boost_DIR ${BOOST_CONFIG_PATH} CACHE INTERNAL "path to Boost config file")

message(STATUS "Using Boost config: ${BOOST_CONFIG_FILE}")

find_package(Boost CONFIG REQUIRED COMPONENTS program_options)

message(STATUS "Boost::program_options found")

# --- Global settings ---

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.fetchcontent")

message(STATUS "FetchContent base dir: ${FETCHCONTENT_BASE_DIR}")

# --- External dependencies ---

message(STATUS "Fetching external dependencies...")

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

message(STATUS "Dependencies fetched: tomlplusplus, spdlog")

# --- toml++ implementation unit ---
# NOTE: forces non-header-only mode to reduce compile times

file(GENERATE OUTPUT _toml_impl.cpp
     CONTENT [=[
#define TOML_HEADER_ONLY 0
#define TOML_IMPLEMENTATION
#define TOML_ENABLE_FORMATTERS 0
#include <toml++/toml.hpp>
     ]=]
)

# --- Core library ---

set(USE_PCH TRUE CACHE BOOL "turn on to use pch")
message(STATUS "Precompiled headers: ${USE_PCH}")

add_library(
    Core 
    STATIC
    src/Logging.h 
    src/Logging.cpp
    src/Config.h 
    src/Config.cpp 
    src/Misc.h 
    src/Misc.cpp
    src/CsvSetParser.h
    src/Algorithms.h
    _toml_impl.cpp
)

if(USE_PCH)
    target_sources(
        Core 
        PRIVATE
            "src/default_pch.h"
    )
endif()

target_link_libraries(
    Core 
    PUBLIC 
        Boost::program_options 
        spdlog::spdlog 
        tomlplusplus::tomlplusplus
)

target_include_directories(
    Core 
    PUBLIC 
        src
)

# NOTE: PCH is shared across targets
target_precompile_headers(
    Core 
    PUBLIC 
        $<$<BOOL:USE_PCH>:"default_pch.h">
)

# NOTE: Disable exceptions in toml++ and enable full spdlog verbosity
target_compile_definitions(
    Core 
    PUBLIC 
        TOML_EXCEPTIONS=0
        TOML_HEADER_ONLY=0
        TOML_ENABLE_FORMATTERS=0
        SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)

set_source_files_properties(
    _toml_impl.cpp
    PROPERTIES 
        SKIP_PRECOMPILE_HEADERS TRUE
)

message(STATUS "Core library configured")

# --- Main executable ---

file(GLOB_RECURSE 
    _data_files 
    CONFIGURE_DEPENDS
    data/* 
)

add_executable(
    Main 
    src/main.cpp 
)

target_link_libraries(
    Main 
    PRIVATE 
        Core
)

target_precompile_headers(
    Main 
    REUSE_FROM 
    Core
)

message(STATUS "Main executable configured")

# --- Data copy helper target ---
# Copies input data to output directory (useful for IDE runs)

add_custom_target(
    CopyData 
    COMMAND ${CMAKE_COMMAND} -E make_directory $<IF:$<CONFIG:Debug>,${DEBUG_BIN_DIR},${RELEASE_BIN_DIR}>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_data_files} $<IF:$<CONFIG:Debug>,${DEBUG_BIN_DIR},${RELEASE_BIN_DIR}> 
    DEPENDS ${_data_files}
    COMMENT "Copying data files..."
    VERBATIM 
    COMMAND_EXPAND_LISTS
    SOURCES ${_data_files}
)

set_target_properties(
    Main 
    PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Main>"
        VS_DEBUGGER_COMMAND_ARGUMENTS "-cfg foo.toml"
)

# --- Tests ---

set(TEST_MedianCalculator TRUE CACHE BOOL "turn off to drop tests")

message(STATUS "Tests enabled: ${TEST_MedianCalculator}")

get_target_property(_main_src Main SOURCES)
get_target_property(_core_src Core SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_main_src})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_core_src})

set_target_properties(spdlog PROPERTIES FOLDER ExternalLibs)

if(TEST_MedianCalculator AND PROJECT_IS_TOP_LEVEL)
    message(STATUS "Configuring tests...")

    include(GoogleTest)   
    enable_testing()

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        SYSTEM
        EXCLUDE_FROM_ALL
    )
    
    FetchContent_MakeAvailable(googletest)

    add_executable(
        Test 
        test/CsvSetParserTests.cpp
        test/ConfigTests.cpp
        test/MiscTests.cpp
        test/AlgorithmsTests.cpp
        test/test_main.cpp
    )

    target_include_directories(Test PRIVATE test)

    target_link_libraries(
        Test 
        PRIVATE 
            GTest::gtest
            Core
    )
   
    gtest_discover_tests(
        Test
        DISCOVERY_MODE PRE_TEST
    )

    target_precompile_headers(Test 
        REUSE_FROM 
        Core
    )

    set_target_properties(
        Test 
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Test>"
    )

    get_target_property(_test_src Test SOURCES)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_test_src})

    set_target_properties(gtest_main gtest PROPERTIES FOLDER ExternalLibs)

    set(TESTS_DROP_LOGGING FALSE CACHE BOOL "turn on to drop logging in tests")

    message(STATUS "Tests logging disabled: ${TESTS_DROP_LOGGING}")

    target_compile_definitions(
        Test 
        PRIVATE
            $<$<BOOL:${TESTS_DROP_LOGGING}>:DROP_LOGGING>
    )

    message(STATUS "Tests configured")
endif()

message(STATUS "Configuration completed successfully")