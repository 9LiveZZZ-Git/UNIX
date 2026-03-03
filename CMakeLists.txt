cmake_minimum_required(VERSION 3.15)
project(UNIX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Options
# =============================================================================
option(BUILD_TESTS "Build test targets" OFF)

# =============================================================================
# Plugin Targets
# =============================================================================
add_subdirectory(KarplusStrongPlugin)
add_subdirectory(SDFSynthPlugin)
