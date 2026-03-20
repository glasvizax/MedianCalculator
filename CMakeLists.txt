cmake_minimum_required(VERSION 3.23)

include(FetchContent)
cmake_policy(SET CMP0167 NEW)
cmake_policy(SET CMP0135 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)

project(MedianCalculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(DEBUG_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug)
set(RELEASE_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/Release)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEBUG_BIN_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RELEASE_BIN_DIR})

set(Boost_DIR "" CACHE PATH "path to Boost config file")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

# -- Core -- 

file(GENERATE OUTPUT _toml_impl.cpp
     CONTENT [=[
#define TOML_HEADER_ONLY 0
#define TOML_IMPLEMENTATION
#define TOML_ENABLE_FORMATTERS 0
#include <toml++/toml.hpp>
     ]=]
)

set(USE_PCH FALSE CACHE BOOL "turn on to use pch")

add_library(
    Core 
    STATIC
    src/Logging.h 
    src/Logging.cpp
    src/Config.h 
    src/Config.cpp 
    src/Misc.h 
    src/Misc.cpp

    _toml_impl.cpp
    $<$<BOOL:USE_PCH>:src/default_pch.h>
)

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

target_precompile_headers(
    Core 
    PUBLIC 
        $<$<BOOL:USE_PCH>:"default_pch.h">
)

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

# -- Main -- 

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

add_custom_target(
    CopyData 
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_data_files} $<IF:$<CONFIG:Debug>,${DEBUG_BIN_DIR},${RELEASE_BIN_DIR}> 
    DEPENDS ${_data_files}
    COMMENT "copying all toml files..."
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

# -- Tests -- 

set(TEST_MedianCalculator TRUE CACHE BOOL "turn off to drop tests")

get_target_property(_main_src Main SOURCES)
get_target_property(_core_src Core SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_main_src})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_core_src})

set_target_properties(spdlog PROPERTIES FOLDER ExternalLibs)

if(TEST_MedianCalculator AND PROJECT_IS_TOP_LEVEL)
    include(GoogleTest)   
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

        
    add_executable(
        Test 
        test/ConfigTests.cpp
        test/MiscTests.cpp
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

    target_compile_definitions(
        Test 
        PRIVATE
            $<$<BOOL:${TESTS_DROP_LOGGING}>:DROP_LOGGING>
    )

endif()

