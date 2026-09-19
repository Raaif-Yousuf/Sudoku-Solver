add_executable(sudoku_bench bench.cpp)
target_link_libraries(sudoku_bench PRIVATE sudoku::sudoku sudoku_warnings)

# Statically link the runtime on MinGW so sudoku_bench.exe does not depend on
# finding the right libstdc++-6.dll / libgcc_s_seh-1.dll on PATH at run time.
# On this toolchain, a shell whose PATH lists another MSYS2 subsystem (e.g.
# /mingw64/bin) ahead of the one the compiler used (e.g. ucrt64) loads an
# ABI-incompatible runtime DLL and crashes intermittently depending on ASLR;
# static linking sidesteps that class of problem entirely for this binary.
if(MINGW)
    target_link_options(sudoku_bench PRIVATE -static-libgcc -static-libstdc++ -static)
endif()
