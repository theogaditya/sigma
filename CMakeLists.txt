cmake_minimum_required(VERSION 3.16)
project(sigma-lang VERSION 0.1.0 LANGUAGES CXX C)

# Require C++17 for std::variant and other modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# Find LLVM using llvm-config (more reliable)
# ============================================================================
find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config REQUIRED)

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

message(STATUS "Found LLVM ${LLVM_VERSION}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIR}")

# Add LLVM include directory
include_directories(${LLVM_INCLUDE_DIR})

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/codegen/CodeGen.cpp
)

# Header files (for IDE support)
set(HEADERS
    src/lexer/Token.h
    src/lexer/Lexer.h
    src/ast/AST.h
    src/ast/ASTPrinter.h
    src/parser/Parser.h
    src/codegen/CodeGen.h
    src/utils/Error.h
)

# Create executable
add_executable(sigma ${SOURCES} ${HEADERS})

# Add LLVM compile flags (filter out incompatible ones)
string(REPLACE "-fno-exceptions" "" LLVM_CXX_FLAGS "${LLVM_CXX_FLAGS}")
target_compile_options(sigma PRIVATE ${LLVM_CXX_FLAGS})

# Link LLVM shared library directly
target_link_libraries(sigma LLVM)
target_link_options(sigma PRIVATE ${LLVM_LD_FLAGS})

# Output to bin directory
set_target_properties(sigma PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Print build info
message(STATUS "Sigma Language Compiler")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
