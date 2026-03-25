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

include(${PROJECT_SOURCE_DIR}/caffeine-build/cmake/CaffeineMacros.cmake)

# Useful for IDEs, clang-tidy, and clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

# --- Dependencies ---
include(FetchContent)
FetchContent_Declare(
    caffeine-hal
    GIT_REPOSITORY https://github.com/while-one/caffeine-hal.git
    GIT_TAG        v0.4.12
)
FetchContent_MakeAvailable(caffeine-hal)

FetchContent_Declare(
    caffeine-sal
    GIT_REPOSITORY https://github.com/while-one/caffeine-sal.git
    GIT_TAG        v0.1.2
)
FetchContent_MakeAvailable(caffeine-sal)

# --- Component Configuration ---
file(GLOB_RECURSE PORT_SOURCES_C "src/**/*.c")
file(GLOB_RECURSE PORT_SOURCES_H "src/**/*.h")

add_library(${PROJECT_NAME} STATIC ${PORT_SOURCES_C})

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
option(CFN_SAL_IMPL_BUILD_TESTS "Build the Caffeine-Services-Impl test executable" OFF)

if(PROJECT_IS_TOP_LEVEL AND CFN_SAL_IMPL_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        # Fetch hal-ports for the mock interface during testing
        FetchContent_Declare(
            caffeine-hal-ports
            GIT_REPOSITORY https://github.com/while-one/caffeine-hal-ports.git
            GIT_TAG        main
        )
        FetchContent_MakeAvailable(caffeine-hal-ports)
        
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

# Gather sources for formatting and analysis
file(GLOB_RECURSE TEST_SOURCES_CPP "tests/*.cpp")
set(ALL_FORMAT_SOURCES ${PORT_SOURCES_C} ${PORT_SOURCES_H} ${TEST_SOURCES_CPP})
set(ALL_ANALYSIS_SOURCES ${PORT_SOURCES_C} ${TEST_SOURCES_CPP})

# 1. Code Formatting (clang-format)
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    option(FORMAT_DRY_RUN "Run clang-format in dry-run mode" OFF)
    
    if(FORMAT_DRY_RUN)
        set(FORMAT_ARGS --dry-run --Werror)
        set(FORMAT_COMMENT "Checking formatting with clang-format (BARR-C style)...")
    else()
        set(FORMAT_ARGS -i)
        set(FORMAT_COMMENT "Formatting files with clang-format (BARR-C style)...")
    endif()

    if(ALL_FORMAT_SOURCES)
        add_custom_target(
            ${PROJECT_NAME}-format
            COMMAND ${CLANG_FORMAT}
            ${FORMAT_ARGS}
            -style=file:${PROJECT_SOURCE_DIR}/caffeine-build/config/coding/.clang-format
            ${ALL_FORMAT_SOURCES}
            COMMENT "${FORMAT_COMMENT}"
        )
    endif()
else()
    message(WARNING "clang-format not found.")
endif()

# 2. Static Analysis (cppcheck)
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    add_custom_target(
        ${PROJECT_NAME}-cppcheck
        COMMAND ${CPPCHECK}
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        --enable=all
        --suppress=missingIncludeSystem
        --suppress=unusedFunction
        --suppress=*:*/_deps/*
        --inline-suppr
        --check-level=exhaustive
        -D__GNUC__
        -I ${caffeine-hal_SOURCE_DIR}/include
        -I ${caffeine-sal_SOURCE_DIR}/include
        --file-filter=${PROJECT_SOURCE_DIR}/src/*
        COMMENT "Running cppcheck static analysis using compile_commands.json..."
    )
    if(NOT TARGET ${PROJECT_NAME}-analyze)
        add_custom_target(${PROJECT_NAME}-analyze COMMENT "Running full static analysis suite...")
    endif()
    add_dependencies(${PROJECT_NAME}-analyze ${PROJECT_NAME}-cppcheck)
endif()

# 3. Static Analysis (clang-tidy)
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY)
    add_custom_target(
        ${PROJECT_NAME}-tidy
        COMMAND ${CLANG_TIDY}
        -p=${CMAKE_BINARY_DIR}
        --config-file=${PROJECT_SOURCE_DIR}/caffeine-build/config/coding/.clang-tidy
        --header-filter="${PROJECT_SOURCE_DIR}/src/.*"
        ${PORT_SOURCES_C}
        COMMENT "Running clang-tidy static analysis using compile_commands.json..."
    )
    
    if(TARGET ${PROJECT_NAME}-analyze)
        add_dependencies(${PROJECT_NAME}-analyze ${PROJECT_NAME}-tidy)
    endif()
endif()

# 4. C11 Compliance Check
add_executable(${PROJECT_NAME}-compliance ${PROJECT_SOURCE_DIR}/tests/compliance.c)
target_link_libraries(${PROJECT_NAME}-compliance PRIVATE ${PROJECT_NAME})
target_compile_options(${PROJECT_NAME}-compliance PRIVATE
    -std=c11
    -pedantic-errors
    -Wall
    -Wextra
    -Werror
)
set_target_properties(${PROJECT_NAME}-compliance PROPERTIES EXCLUDE_FROM_ALL ON)

if(TARGET ${PROJECT_NAME}-analyze)
    add_dependencies(${PROJECT_NAME}-analyze ${PROJECT_NAME}-compliance)
endif()
