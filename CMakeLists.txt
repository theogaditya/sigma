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
    add_compile_options(/W4)
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
# Find LLVM using llvm-config (more reliable)
# ============================================================================
find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config llvm-config-18 llvm-config-17 REQUIRED)

# Get LLVM flags
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

execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE LLVM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get LLVM libraries for static linking
if(SIGMA_STATIC)
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --link-static
        OUTPUT_VARIABLE LLVM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
        OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIR}")
message(STATUS "Static linking: ${SIGMA_STATIC}")

# Add LLVM include directory
include_directories(${LLVM_INCLUDE_DIR})

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/semantic/TypeChecker.cpp
    src/codegen/CodeGen.cpp
)

# Header files (for IDE support)
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

# Create executable
add_executable(sigma ${SOURCES} ${HEADERS})

# Add LLVM compile flags (filter out incompatible ones)
string(REPLACE "-fno-exceptions" "" LLVM_CXX_FLAGS "${LLVM_CXX_FLAGS}")
target_compile_options(sigma PRIVATE ${LLVM_CXX_FLAGS})

# Link LLVM (static or dynamic)
if(SIGMA_STATIC)
    # Static linking for portable binaries
    separate_arguments(LLVM_LIBS_LIST NATIVE_COMMAND "${LLVM_LIBS}")
    separate_arguments(LLVM_SYSTEM_LIBS_LIST NATIVE_COMMAND "${LLVM_SYSTEM_LIBS}")
    target_link_libraries(sigma ${LLVM_LIBS_LIST} ${LLVM_SYSTEM_LIBS_LIST})
    if(NOT MSVC)
        target_link_options(sigma PRIVATE -static-libgcc -static-libstdc++)
    endif()
else()
    # Dynamic linking (development)
    target_link_libraries(sigma LLVM)
endif()
target_link_options(sigma PRIVATE ${LLVM_LD_FLAGS})

# Output to bin directory
set_target_properties(sigma PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# ============================================================================
# Test Target
# ============================================================================

# Test sources (Lexer and Parser only - no LLVM dependency)
set(TEST_SOURCES
    tests/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
)

# Create test executable
add_executable(sigma_tests ${TEST_SOURCES})

# Include directories for tests
target_include_directories(sigma_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Output tests to bin directory
set_target_properties(sigma_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Enable testing
enable_testing()
add_test(NAME SigmaTests COMMAND sigma_tests)

# Print build info
message(STATUS "Sigma Language Compiler")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
