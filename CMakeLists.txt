cmake_minimum_required(VERSION 3.16)
project(SnipCLI 
    VERSION 1.0.0 
    LANGUAGES CXX 
    DESCRIPTION "A fast, modern C++ CLI snippet manager for competitive programmers"
)

# Standard C++17 Enforcement
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd / language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler Warning Flags
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wshadow 
        -Wnon-virtual-dtor 
        -Wold-style-cast 
        -Wcast-align 
        -Wunused 
        -Woverloaded-virtual 
        -Wconversion 
        -Wsign-conversion
    )
endif()

# Directories
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Include Third-Party / Single-Header Libs
include_directories(${INCLUDE_DIR})

# Library Target (Core Domain, View, Controller)
add_library(snipcli_core
    ${SRC_DIR}/model/snippet.cpp
    ${SRC_DIR}/model/snippet_validator.cpp
    ${SRC_DIR}/model/json_snippet_repository.cpp
    ${SRC_DIR}/model/strategy/tag_search_strategy.cpp
    ${SRC_DIR}/model/strategy/title_search_strategy.cpp
    ${SRC_DIR}/view/terminal_view.cpp
    ${SRC_DIR}/view/markdown_view.cpp
    ${SRC_DIR}/view/view_factory.cpp
    ${SRC_DIR}/controller/cli_parser.cpp
    ${SRC_DIR}/controller/command_dispatcher.cpp
    ${SRC_DIR}/controller/commands/add_command.cpp
    ${SRC_DIR}/controller/commands/list_command.cpp
    ${SRC_DIR}/controller/commands/get_command.cpp
    ${SRC_DIR}/controller/commands/delete_command.cpp
    ${SRC_DIR}/controller/commands/search_command.cpp
    ${SRC_DIR}/controller/commands/stats_command.cpp
)

target_include_directories(snipcli_core PUBLIC ${INCLUDE_DIR})

# Executable Target
add_executable(snip ${SRC_DIR}/main.cpp)
target_link_libraries(snip PRIVATE snipcli_core)

# CTest / Unit Testing Target
enable_testing()
add_executable(snip_tests 
    tests/unit/test_main.cpp
    tests/unit/test_snippet.cpp
    tests/unit/test_validator.cpp
    tests/unit/test_repository.cpp
    tests/unit/test_strategy.cpp
    tests/unit/test_parser.cpp
)
target_link_libraries(snip_tests PRIVATE snipcli_core)
add_test(NAME CoreUnitTests COMMAND snip_tests)
