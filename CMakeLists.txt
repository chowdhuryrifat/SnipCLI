cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(SnipCLI 
    VERSION 1.0.0 
    LANGUAGES CXX 
    DESCRIPTION "A fast, modern C++ CLI snippet manager for competitive programmers"
)

# MSVC Static Runtime: Link C/C++ runtime statically (/MT in Release, /MTd in Debug)
# Ensures standalone Windows binaries with zero external MSVC runtime DLL dependencies
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()


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
    ${SRC_DIR}/model/strategy/language_search_strategy.cpp
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

# Generate version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.hpp
    @ONLY
)

target_include_directories(snipcli_core PUBLIC ${INCLUDE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Executable Target
add_executable(snip ${SRC_DIR}/main.cpp)
target_link_libraries(snip PRIVATE snipcli_core)

# Testing Target
option(SNIPCLI_BUILD_TESTS "Build SnipCLI unit test targets" ON)

if(SNIPCLI_BUILD_TESTS)
    enable_testing()
    add_executable(snip_tests 
        tests/unit/test_main.cpp
        tests/unit/test_snippet.cpp
        tests/unit/test_validator.cpp
        tests/unit/test_repository.cpp
        tests/unit/test_strategy.cpp
        tests/unit/test_parser.cpp
        tests/unit/test_commands.cpp
        tests/unit/test_view.cpp
    )
    target_link_libraries(snip_tests PRIVATE snipcli_core)
    add_test(NAME CoreUnitTests COMMAND snip_tests)
endif()

# Installation Targets (FHS Compliant)
include(GNUInstallDirs)

# Binary
install(TARGETS snip
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Man page
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/man/snip.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

# Shell Completions
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/completions/snip.bash
    DESTINATION ${CMAKE_INSTALL_DATADIR}/bash-completion/completions
    RENAME snip
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/completions/_snip
    DESTINATION ${CMAKE_INSTALL_DATADIR}/zsh/site-functions
)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/completions/snip.fish
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fish/vendor_completions.d
)

# Documentation and License
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    ${CMAKE_CURRENT_SOURCE_DIR}/NOTICE
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "snipcli")
set(CPACK_PACKAGE_VENDOR "Chowdhury Rifat Ahmed & Lameya Tabasum")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast, modern C++17 CLI snippet manager for competitive programmers")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "Chowdhury Rifat Ahmed <67093774+chowdhuryrifat@users.noreply.github.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Debian (.deb) Specifics
set(CPACK_DEBIAN_PACKAGE_NAME "snipcli")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Chowdhury Rifat Ahmed <67093774+chowdhuryrifat@users.noreply.github.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")

# Archive Generators
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")

include(CPack)


