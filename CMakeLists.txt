cmake_minimum_required(VERSION 3.15)
project(Clarity VERSION 0.1.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")

add_executable(Clarity_BMI2 src/board.cpp src/globals.cpp src/globals.h src/bmi2.h src/bmi2.cpp src/move.cpp src/movegen.cpp src/search.h src/search.cpp src/testessentials.h src/tests.cpp src/testsuites.h src/tt.h src/tt.cpp src/tuner.cpp src/tuner.h src/uci.cpp src/nnue.cpp)
add_executable(Clarity_Native src/board.cpp src/globals.cpp src/globals.h src/bmi2.h src/bmi2.cpp src/move.cpp src/movegen.cpp src/search.h src/search.cpp src/testessentials.h src/tests.cpp src/testsuites.h src/tt.h src/tt.cpp src/tuner.cpp src/tuner.h src/uci.cpp src/nnue.cpp)


target_compile_options(Clarity_BMI2 PRIVATE -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-missing-prototypes -Wno-missing-variable-declarations -march=haswell)
target_compile_options(Clarity_Native PRIVATE -Wall -Wextra -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-missing-prototypes -Wno-missing-variable-declarations -march=native)
