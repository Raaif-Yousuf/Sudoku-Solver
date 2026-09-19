cmake_minimum_required(VERSION 3.16)
project(SudokuSolver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SUDOKU_BUILD_TESTS "Build the GoogleTest suite" ON)
option(SUDOKU_BUILD_BENCH "Build the benchmark tool" ON)
option(SUDOKU_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SUDOKU_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(sudoku_warnings INTERFACE)
if(MSVC)
    target_compile_options(sudoku_warnings INTERFACE /W4 /permissive-)
    if(SUDOKU_WARNINGS_AS_ERRORS)
        target_compile_options(sudoku_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(sudoku_warnings INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    if(SUDOKU_WARNINGS_AS_ERRORS)
        target_compile_options(sudoku_warnings INTERFACE -Werror)
    endif()
endif()

if(SUDOKU_SANITIZE AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(sudoku
    src/grid.cpp
    src/backtracking_solver.cpp
    src/solver_factory.cpp
)
add_library(sudoku::sudoku ALIAS sudoku)
target_include_directories(sudoku PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sudoku PRIVATE sudoku_warnings)

add_executable(sudoku_cli app/main.cpp)
set_target_properties(sudoku_cli PROPERTIES OUTPUT_NAME sudoku)
target_link_libraries(sudoku_cli PRIVATE sudoku::sudoku sudoku_warnings)

if(SUDOKU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SUDOKU_BUILD_BENCH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bench/CMakeLists.txt)
    add_subdirectory(bench)
endif()
