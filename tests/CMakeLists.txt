enable_language(CXX)

option(FORCE_FETCH_DEPS "Force downloading dependencies even if found on system" OFF)

if(NOT FORCE_FETCH_DEPS)
    find_package(GTest QUIET)
endif()

if(NOT GTest_FOUND)
    message(STATUS "GTest not found locally, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    # Prevent GTest from overriding the parent project's compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
else()
    message(STATUS "Using System-provided GoogleTest (Skip Download)")
endif()

# Alias GoogleTest targets with project prefix for better identification
add_library(${PROJECT_NAME}-gmock_main INTERFACE)
if(GTest_FOUND)
    target_link_libraries(${PROJECT_NAME}-gmock_main INTERFACE GTest::gmock_main)
else()
    target_link_libraries(${PROJECT_NAME}-gmock_main INTERFACE gmock_main)
endif()

# Create the test executable
# Globbing is recommended for unit tests to ensure total coverage
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_executable(${PROJECT_NAME}-test ${TEST_SOURCES})

# Always enable coverage instrumentation for host-side unit tests
target_compile_options(${PROJECT_NAME}-test PRIVATE --coverage -O0 -g)
target_link_options(${PROJECT_NAME}-test PRIVATE --coverage)

# Link your services library, HAL library, and GoogleTest
target_link_libraries(${PROJECT_NAME}-test
    PRIVATE
    ${PROJECT_NAME}
    caffeine::hal
    caffeine::hal-mock
    caffeine::sal
    ${PROJECT_NAME}-gmock_main
)

# Tell CTest to discover and run all tests in this executable
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}-test)
