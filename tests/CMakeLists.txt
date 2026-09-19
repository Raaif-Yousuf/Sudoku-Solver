include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Use the same C runtime as the rest of the project on MSVC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(SUDOKU_TEST_SOURCES
    test_grid.cpp
    test_solvers.cpp
    test_backtracking_solver.cpp
    test_fast_solver.cpp
)

add_executable(sudoku_tests ${SUDOKU_TEST_SOURCES})
target_link_libraries(sudoku_tests PRIVATE sudoku::sudoku sudoku_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(sudoku_tests DISCOVERY_TIMEOUT 60)

# ---------------------------------------------------------------------------
# CLI tests: exercise the sudoku_cli binary itself through CTest, using a
# tiny cmake -P wrapper so exit-code and output checks work identically on
# Windows, Linux, and macOS (no shell scripting required).
# ---------------------------------------------------------------------------

set(SUDOKU_CLI_TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(SUDOKU_CLI_TEST_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/run_cli_test.cmake)

# sudoku_add_cli_test(<test-name>
#                      ARGS <arg>...
#                      EXPECTED_EXIT <code>
#                      [EXPECTED_REGEX <regex>]
#                      [FORBIDDEN_REGEX <regex>]
#                      [STDERR_REGEX <regex>])
function(sudoku_add_cli_test name)
    cmake_parse_arguments(T "" "EXPECTED_EXIT;EXPECTED_REGEX;FORBIDDEN_REGEX;STDERR_REGEX" "ARGS" ${ARGN})

    if(NOT DEFINED T_EXPECTED_EXIT)
        set(T_EXPECTED_EXIT 0)
    endif()

    set(joined_args "")
    foreach(arg IN LISTS T_ARGS)
        if(joined_args STREQUAL "")
            set(joined_args "${arg}")
        else()
            set(joined_args "${joined_args}@@${arg}")
        endif()
    endforeach()

    set(script_defs
        "-DCLI_EXE=$<TARGET_FILE:sudoku_cli>"
        "-DCLI_ARGS=${joined_args}"
        "-DEXPECTED_EXIT=${T_EXPECTED_EXIT}")
    if(DEFINED T_EXPECTED_REGEX)
        list(APPEND script_defs "-DEXPECTED_REGEX=${T_EXPECTED_REGEX}")
    endif()
    if(DEFINED T_FORBIDDEN_REGEX)
        list(APPEND script_defs "-DFORBIDDEN_REGEX=${T_FORBIDDEN_REGEX}")
    endif()
    if(DEFINED T_STDERR_REGEX)
        list(APPEND script_defs "-DSTDERR_REGEX=${T_STDERR_REGEX}")
    endif()

    add_test(NAME ${name}
             COMMAND ${CMAKE_COMMAND} ${script_defs} -P ${SUDOKU_CLI_TEST_SCRIPT})
endfunction()

sudoku_add_cli_test(cli.help_prints_usage
    ARGS --help
    EXPECTED_EXIT 0
    EXPECTED_REGEX "Usage")

sudoku_add_cli_test(cli.solves_single_grid_file
    ARGS ${SUDOKU_CLI_TEST_DATA_DIR}/single_grid.txt
    EXPECTED_EXIT 0
    FORBIDDEN_REGEX "No solution")

sudoku_add_cli_test(cli.solves_multi_puzzle_file
    ARGS --format line ${SUDOKU_CLI_TEST_DATA_DIR}/multi_puzzle.txt
    EXPECTED_EXIT 0
    FORBIDDEN_REGEX "No solution")

sudoku_add_cli_test(cli.reports_no_solution_for_unsolvable_puzzle
    ARGS --format line ${SUDOKU_CLI_TEST_DATA_DIR}/unsolvable.txt
    EXPECTED_EXIT 1
    EXPECTED_REGEX "No solution")

sudoku_add_cli_test(cli.rejects_malformed_file
    ARGS ${SUDOKU_CLI_TEST_DATA_DIR}/malformed.txt
    EXPECTED_EXIT 2
    STDERR_REGEX "not a valid puzzle")

sudoku_add_cli_test(cli.rejects_missing_file
    ARGS ${SUDOKU_CLI_TEST_DATA_DIR}/does_not_exist.txt
    EXPECTED_EXIT 2
    STDERR_REGEX "cannot open")

sudoku_add_cli_test(cli.rejects_unknown_solver
    ARGS --solver nope ${SUDOKU_CLI_TEST_DATA_DIR}/single_grid.txt
    EXPECTED_EXIT 2
    STDERR_REGEX "unknown solver")

sudoku_add_cli_test(cli.rejects_unknown_option
    ARGS --bogus-option
    EXPECTED_EXIT 2
    STDERR_REGEX "unknown option")

sudoku_add_cli_test(cli.stats_flag_reports_node_count
    ARGS --stats ${SUDOKU_CLI_TEST_DATA_DIR}/single_grid.txt
    EXPECTED_EXIT 0
    STDERR_REGEX "nodes=")

sudoku_add_cli_test(cli.fast_solver_reports_unique_solution
    ARGS --solver fast --unique ${SUDOKU_CLI_TEST_DATA_DIR}/single_grid.txt
    EXPECTED_EXIT 0
    STDERR_REGEX "unique solution")
