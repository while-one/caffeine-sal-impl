cmake_minimum_required(VERSION 3.23)

project(caffeine-sal-impl
        VERSION 0.1.0
        LANGUAGES C CXX
        DESCRIPTION "Concrete implementations for the Caffeine Framework middleware services"
        HOMEPAGE_URL "https://github.com/while-one/caffeine-sal-impl"
)

# Global standards for the Caffeine Framework
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

message("BUILD IS UNIVERSE: ${CAFFEINE_UNIVERSE_TARGET}")

# 3. Code Quality (Formatting & Analysis)
# Define the absolute universe of sources for this repository
# Globbing is encouraged for CI quality gates to ensure 100% coverage
file(GLOB_RECURSE ALL_UNIVERSE_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.c"
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/tests/*.cpp"
    "${PROJECT_SOURCE_DIR}/tests/*.c"
)

include(${PROJECT_SOURCE_DIR}/caffeine-build/cmake/CaffeineMacros.cmake)
cfn_add_universe_targets(INPUTS ${ALL_UNIVERSE_SOURCES})

# Useful for IDEs, clang-tidy, and clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

# --- Dependencies ---
include(FetchContent)
FetchContent_Declare(
    caffeine-hal
    GIT_REPOSITORY https://github.com/while-one/caffeine-hal.git
    GIT_TAG        v0.6.3

)

FetchContent_MakeAvailable(caffeine-hal)

FetchContent_Declare(
    caffeine-sal
    GIT_REPOSITORY https://github.com/while-one/caffeine-sal.git
    GIT_TAG        v0.1.16
)
FetchContent_MakeAvailable(caffeine-sal)

# --- Component Configuration ---
# Actual binary targets MUST explicitly list all sources for determinism
set(CFN_SAL_IMPL_SOURCES_C
    src/devices/accel/bma530/cfn_sal_accel_bma530.c
    src/devices/accel/lis2dh12/cfn_sal_accel_lis2dh12.c
    src/devices/button/cfn_sal_button.c
    src/devices/composite_sensor/aht20/cfn_sal_aht20.c
    src/devices/composite_sensor/bme280/cfn_sal_bme280.c
    src/devices/composite_sensor/lsm6ds3/cfn_sal_lsm6ds3.c
    src/devices/composite_sensor/opt4048/cfn_sal_opt4048.c
    src/devices/composite_sensor/sht30/cfn_sal_sht30.c
    src/devices/composite_sensor/sht40/cfn_sal_sht40.c
    src/devices/composite_sensor/vcnl4040/cfn_sal_vcnl4040.c
    src/devices/gpio_expander/pcf8574/cfn_sal_pcf8574.c
    src/devices/led/cfn_sal_led.c
    src/utilities/cli/cfn_sal_cli.c
    src/utilities/collection/linkedlist/cfn_sal_collection_linkedlist.c
    src/utilities/collection/ringbuffer/cfn_sal_collection_ringbuffer.c
    src/utilities/timekeeping/cfn_sal_timekeeping.c
)

set(CFN_SAL_IMPL_SOURCES_H
    src/devices/accel/bma530/cfn_sal_accel_bma530.h
    src/devices/accel/lis2dh12/cfn_sal_accel_lis2dh12.h
    src/devices/button/cfn_sal_button.h
    src/devices/composite_sensor/aht20/cfn_sal_aht20.h
    src/devices/composite_sensor/bme280/cfn_sal_bme280.h
    src/devices/composite_sensor/lsm6ds3/cfn_sal_lsm6ds3.h
    src/devices/composite_sensor/opt4048/cfn_sal_opt4048.h
    src/devices/composite_sensor/sht30/cfn_sal_sht30.h
    src/devices/composite_sensor/sht40/cfn_sal_sht40.h
    src/devices/composite_sensor/vcnl4040/cfn_sal_vcnl4040.h
    src/devices/gpio_expander/pcf8574/cfn_sal_pcf8574.h
    src/devices/led/cfn_sal_led.h
    src/utilities/cli/cfn_sal_cli.h
    src/utilities/collection/linkedlist/cfn_sal_collection_linkedlist.h
    src/utilities/collection/ringbuffer/cfn_sal_collection_ringbuffer.h
)

add_library(${PROJECT_NAME} STATIC ${CFN_SAL_IMPL_SOURCES_C})

target_compile_options(${PROJECT_NAME} PRIVATE ${CAFFEINE_COMPILE_OPTIONS})
target_compile_options(${PROJECT_NAME} INTERFACE ${CAFFEINE_COMPILE_OPTIONS})

target_link_libraries(${PROJECT_NAME} PUBLIC caffeine::hal caffeine::sal)

target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

add_library(caffeine::sal-impl ALIAS ${PROJECT_NAME})

# Consistent naming for exported target
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME services-impl)

# --- Testing ---
option(CFN_BUILD_TESTS "Build unit tests" OFF)

if(PROJECT_IS_TOP_LEVEL AND CFN_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

# --- Installation ---
install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Prepare the version information
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

# Configure the input and output of the config file that will be generated
configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# Store the ${PROJECT_NAME}_Targets generated previously in the config file
install(
        EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE caffeine::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# Copy the generic config file and the version config file to the installation destination
install(
        FILES
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# ==============================================================================
# Tooling: Formatting & Analysis
# ==============================================================================

# Use globbing for quality gate sources
file(GLOB_RECURSE ALL_ANALYSIS_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

cfn_add_code_quality_targets(
    ${PROJECT_NAME}
    ANALYSIS_SOURCES ${ALL_ANALYSIS_SOURCES}
    CPPCHECK_FILE_FILTER "${PROJECT_SOURCE_DIR}/src/*"
    TIDY_HEADER_FILTER "${PROJECT_SOURCE_DIR}/src/.*"
)

# 5. Doxygen Documentation
set(ALL_UNIVERSE_SOURCES_NO_TESTS ${ALL_UNIVERSE_SOURCES})
list(FILTER ALL_UNIVERSE_SOURCES_NO_TESTS EXCLUDE REGEX ".*/tests/.*")
cfn_add_docs(
    ${PROJECT_NAME}-docs
    INPUTS ${ALL_UNIVERSE_SOURCES_NO_TESTS} ${PROJECT_SOURCE_DIR}/README.md
    COMMENT "Generating API documentation with Doxygen"
)
