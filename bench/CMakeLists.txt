add_executable(sudoku_bench bench.cpp)
target_link_libraries(sudoku_bench PRIVATE sudoku::sudoku sudoku_warnings)

# Link the MinGW runtime statically so timings do not depend on which
# libstdc++ DLL happens to be first on PATH.
if(MINGW)
    target_link_options(sudoku_bench PRIVATE -static-libgcc -static-libstdc++ -static)
endif()
