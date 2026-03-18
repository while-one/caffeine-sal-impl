cmake_minimum_required(VERSION 3.23)

# Placeholder project name and version
project(caffeine-template
        VERSION 0.1.3
        LANGUAGES C
        DESCRIPTION "Replace me with a good description"
        HOMEPAGE_URL "https://github.com/while-one/caffeine-template"
)

# Global standards for the Caffeine Framework
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Useful for IDEs, clang-tidy, and clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Component Configuration ---
# add_library(caffeine_<name> STATIC src/main.c)
# target_include_directories(caffeine_<name> PUBLIC include)

# --- Optional: Add unit tests ---
# enable_testing()
# add_subdirectory(tests)
