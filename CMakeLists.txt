cmake_minimum_required(VERSION 3.20)

project(multicore_cpu_scheduler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(scheduler_core
    src/process.cpp
    src/cpu.cpp
    src/run_queue.cpp
    src/event.cpp
    src/simulation.cpp
    schedulers/fcfs.cpp
    schedulers/round_robin.cpp
)

target_include_directories(scheduler_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(scheduler_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(scheduler_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(scheduler_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(scheduler src/main.cpp)
target_link_libraries(scheduler PRIVATE scheduler_core)

include(CTest)
option(SCHEDULER_BUILD_TESTS "Build GoogleTest tests" ON)

if(BUILD_TESTING AND SCHEDULER_BUILD_TESTS)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "Use the shared MSVC runtime" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(scheduler_tests
        tests/process_test.cpp
        tests/cpu_test.cpp
        tests/simulation_test.cpp
        tests/fcfs_test.cpp
        tests/round_robin_test.cpp
    )
    target_link_libraries(scheduler_tests PRIVATE scheduler_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(scheduler_tests)
endif()
