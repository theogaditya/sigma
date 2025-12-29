cmake_minimum_required(VERSION 3.16)
project(sigma-lang VERSION 1.1.0 LANGUAGES CXX C)

# Require C++17 for std::variant and other modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type (Release for distribution)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Option for static linking (for portable binaries)
option(SIGMA_STATIC "Build statically linked binary" OFF)

# Enable warnings
if(MSVC)
    add_compile_options(/W4 /EHsc)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

# ============================================================================
# Find LLVM - Try multiple methods for cross-platform support
# ============================================================================

# Method 1: Try llvm-config first (Linux/macOS - allows version control)
find_program(LLVM_CONFIG_EXECUTABLE 
    NAMES llvm-config-18 llvm-config-17 llvm-config-19 llvm-config
)

if(LLVM_CONFIG_EXECUTABLE AND NOT WIN32)
    message(STATUS "Found llvm-config: ${LLVM_CONFIG_EXECUTABLE}")
    
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
        OUTPUT_VARIABLE LLVM_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "LLVM version: ${LLVM_VERSION}")
    
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
        OUTPUT_VARIABLE LLVM_CXX_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
        OUTPUT_VARIABLE LLVM_LD_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
        OUTPUT_VARIABLE LLVM_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    include_directories(${LLVM_INCLUDE_DIR})
    
    # For static linking
    if(SIGMA_STATIC)
        execute_process(
            COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --link-static
            OUTPUT_VARIABLE LLVM_LIBS_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
            OUTPUT_VARIABLE LLVM_SYSTEM_LIBS_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        separate_arguments(LLVM_LIBS_LIST NATIVE_COMMAND "${LLVM_LIBS_RAW}")
        separate_arguments(LLVM_SYSTEM_LIBS_LIST NATIVE_COMMAND "${LLVM_SYSTEM_LIBS_RAW}")
        set(LLVM_LINK_LIBS ${LLVM_LIBS_LIST} ${LLVM_SYSTEM_LIBS_LIST})
    else()
        set(LLVM_LINK_LIBS LLVM)
    endif()
    
    set(LLVM_FOUND_VIA "llvm-config")

else()
    # Method 2: Use CMake's find_package (Windows or when llvm-config not found)
    find_package(LLVM CONFIG REQUIRED)
    
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} via CMake config")
    message(STATUS "LLVM installed in: ${LLVM_INSTALL_PREFIX}")
    
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})
    
    # Get the libraries we need
    llvm_map_components_to_libnames(LLVM_LIBS 
        Core
        Support
        IRReader
        BitWriter
        Passes
        Target
        Analysis
        TransformUtils
        ScalarOpts
        InstCombine
        native
    )
    
    set(LLVM_LINK_LIBS ${LLVM_LIBS})
    set(LLVM_CXX_FLAGS "")
    set(LLVM_LD_FLAGS "")
    set(LLVM_FOUND_VIA "CMake")
endif()

message(STATUS "Static linking: ${SIGMA_STATIC}")

# ============================================================================
# Source files
# ============================================================================
include_directories(${CMAKE_SOURCE_DIR}/src)

set(SOURCES
    src/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/semantic/TypeChecker.cpp
    src/codegen/CodeGen.cpp
)

set(HEADERS
    src/lexer/Token.h
    src/lexer/Lexer.h
    src/ast/AST.h
    src/ast/ASTPrinter.h
    src/parser/Parser.h
    src/semantic/Types.h
    src/semantic/SymbolTable.h
    src/semantic/TypeChecker.h
    src/codegen/CodeGen.h
    src/utils/Error.h
)

# ============================================================================
# Main executable
# ============================================================================
add_executable(sigma ${SOURCES} ${HEADERS})

# Apply LLVM compile flags (filter out incompatible ones)
if(LLVM_CXX_FLAGS)
    string(REPLACE "-fno-exceptions" "" LLVM_CXX_FLAGS "${LLVM_CXX_FLAGS}")
    string(REPLACE "-fno-rtti" "" LLVM_CXX_FLAGS "${LLVM_CXX_FLAGS}")
    separate_arguments(LLVM_CXX_FLAGS_LIST NATIVE_COMMAND "${LLVM_CXX_FLAGS}")
    target_compile_options(sigma PRIVATE ${LLVM_CXX_FLAGS_LIST})
endif()

# Link LLVM
target_link_libraries(sigma ${LLVM_LINK_LIBS})

# Apply linker flags
if(LLVM_LD_FLAGS)
    separate_arguments(LLVM_LD_FLAGS_LIST NATIVE_COMMAND "${LLVM_LD_FLAGS}")
    target_link_options(sigma PRIVATE ${LLVM_LD_FLAGS_LIST})
endif()

# Static linking options for GCC/Clang
if(SIGMA_STATIC AND NOT MSVC)
    target_link_options(sigma PRIVATE -static-libgcc -static-libstdc++)
endif()

# Output directory
set_target_properties(sigma PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Test executable (no LLVM dependency)
# ============================================================================
set(TEST_SOURCES
    tests/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
)

add_executable(sigma_tests ${TEST_SOURCES})

target_include_directories(sigma_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

set_target_properties(sigma_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Testing
# ============================================================================
enable_testing()
add_test(NAME SigmaTests COMMAND sigma_tests)

# ============================================================================
# Build info
# ============================================================================
message(STATUS "")
message(STATUS "=== Sigma Language Compiler ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
