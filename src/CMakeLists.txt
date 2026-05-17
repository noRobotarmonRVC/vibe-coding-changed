cmake_minimum_required(VERSION 3.16)
project(RvcControlSW CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_COVERAGE "Enable gcovr coverage" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0)
    add_link_options(--coverage)
endif()

# ── core library (all non-main translation units) ─────────────────────────────
add_library(rvc_core
    domain/DefaultNavigationStrategy.cpp
    hal/FrontSensor.cpp
    hal/LeftSensor.cpp
    hal/RightSensor.cpp
    hal/DustSensor.cpp
    app/RvcController.cpp
    simulator/Simulator.cpp
    ui/ConsoleDisplay.cpp
    ui/GridDisplay.cpp
)
target_include_directories(rvc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ── main executable ───────────────────────────────────────────────────────────
add_executable(rvc_main app/main.cpp)
target_link_libraries(rvc_main PRIVATE rvc_core)

# ── unit + integration tests ──────────────────────────────────────────────────
enable_testing()
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(rvc_tests
    ../test/domain/DefaultNavigationStrategyTest.cpp
    ../test/app/RvcControllerTest.cpp
    ../test/simulator/SimulatorTest.cpp
)
target_link_libraries(rvc_tests PRIVATE rvc_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(rvc_tests)
