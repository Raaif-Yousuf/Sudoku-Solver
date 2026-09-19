# Runs sudoku_cli with a fixed set of arguments and checks its exit code and
# (optionally) its stdout/stderr against regular expressions. Used from
# add_test() via `cmake -P` so the check works the same way on every
# platform CI runs on, without relying on a shell.
#
# Expected variables (set with -D):
#   CLI_EXE          path to the sudoku_cli executable (required)
#   CLI_ARGS         "@@"-separated argument list (may be empty)
#   EXPECTED_EXIT    the exit code sudoku_cli must return (required)
#   EXPECTED_REGEX   stdout must match this regex, if set
#   FORBIDDEN_REGEX  stdout must NOT match this regex, if set
#   STDERR_REGEX     stderr must match this regex, if set

if(NOT DEFINED CLI_EXE)
    message(FATAL_ERROR "CLI_EXE was not set")
endif()
if(NOT DEFINED EXPECTED_EXIT)
    message(FATAL_ERROR "EXPECTED_EXIT was not set")
endif()

set(cli_args "")
if(DEFINED CLI_ARGS AND NOT CLI_ARGS STREQUAL "")
    string(REPLACE "@@" ";" cli_args "${CLI_ARGS}")
endif()

execute_process(
    COMMAND "${CLI_EXE}" ${cli_args}
    OUTPUT_VARIABLE cli_stdout
    ERROR_VARIABLE cli_stderr
    RESULT_VARIABLE cli_result
)

if(NOT cli_result EQUAL EXPECTED_EXIT)
    message(FATAL_ERROR
        "expected exit code ${EXPECTED_EXIT} but got ${cli_result}\n"
        "--- stdout ---\n${cli_stdout}\n"
        "--- stderr ---\n${cli_stderr}")
endif()

if(DEFINED EXPECTED_REGEX AND NOT cli_stdout MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR
        "stdout did not match expected regex: ${EXPECTED_REGEX}\n"
        "--- stdout ---\n${cli_stdout}")
endif()

if(DEFINED FORBIDDEN_REGEX AND cli_stdout MATCHES "${FORBIDDEN_REGEX}")
    message(FATAL_ERROR
        "stdout matched forbidden regex: ${FORBIDDEN_REGEX}\n"
        "--- stdout ---\n${cli_stdout}")
endif()

if(DEFINED STDERR_REGEX AND NOT cli_stderr MATCHES "${STDERR_REGEX}")
    message(FATAL_ERROR
        "stderr did not match expected regex: ${STDERR_REGEX}\n"
        "--- stderr ---\n${cli_stderr}")
endif()
