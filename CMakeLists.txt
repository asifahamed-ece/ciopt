cmake_minimum_required(VERSION 3.12)
project(ciopt VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files variables
file(GLOB CIOPT_SOURCES
    src/ciopt/utils/*.c
    src/ciopt/analyzer/*.c
    src/ciopt/reporting/*.c
    src/ciopt/parser/*.c
    src/ciopt/api.c
    src/ciopt/config.c
)

# Tree-sitter files
set(TS_SRC vendor/tree-sitter/lib/src/lib.c)
set(TS_C_SRC vendor/tree-sitter-c/src/parser.c)

# Include directories
include_directories(
    src
    vendor/tree-sitter/lib/include
    vendor/tree-sitter-c/src
)

# Build Tree-Sitter
add_library(tree-sitter STATIC ${TS_SRC})
target_compile_definitions(tree-sitter PRIVATE _POSIX_C_SOURCE=200809L _DEFAULT_SOURCE HAVE_TREE_SITTER=1)
target_include_directories(tree-sitter PRIVATE vendor/tree-sitter/lib/src)

# Build Tree-Sitter C Grammar
add_library(tree-sitter-c STATIC ${TS_C_SRC})
target_compile_definitions(tree-sitter-c PRIVATE _POSIX_C_SOURCE=200809L _DEFAULT_SOURCE HAVE_TREE_SITTER=1)
target_include_directories(tree-sitter-c PRIVATE vendor/tree-sitter-c/src)

# Build Core ciopt static library
add_library(ciopt_core STATIC ${CIOPT_SOURCES})
target_compile_definitions(ciopt_core PUBLIC _POSIX_C_SOURCE=200809L _DEFAULT_SOURCE HAVE_TREE_SITTER=1)
target_link_libraries(ciopt_core PUBLIC tree-sitter tree-sitter-c)

# Build main CLI executable
add_executable(ciopt src/main.c)
target_link_libraries(ciopt PRIVATE ciopt_core)

# Enable testing
enable_testing()

# Build vector tests
add_executable(test_vector tests/test_vector.c)
target_link_libraries(test_vector PRIVATE ciopt_core)
add_test(NAME test_vector COMMAND test_vector)

# Build analyzer tests
add_executable(test_analyzer tests/test_analyzer.c)
target_link_libraries(test_analyzer PRIVATE ciopt_core)
add_test(NAME test_analyzer COMMAND test_analyzer)
