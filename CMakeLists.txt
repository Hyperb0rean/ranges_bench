cmake_minimum_required(VERSION 3.12)

project(ranges_benches)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

include(FetchContent)

set(BENCHMARK_ENABLE_TESTING off)   
FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main)

FetchContent_Declare(googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main) 

FetchContent_MakeAvailable(googletest googlebenchmark)



# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -mavx512f")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_compile_options(-stdlib=libc++)

add_link_options(-stdlib=libc++)

add_executable(ranges ranges.cpp)

target_link_libraries(ranges benchmark::benchmark)
