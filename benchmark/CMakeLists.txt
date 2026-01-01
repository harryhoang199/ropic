# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(ropic-benchmarks LANGUAGES CXX)

###############################################################################
# Fetch Google Benchmark                                                      #
###############################################################################
include(FetchContent)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.4

  GIT_SHALLOW ON
  SYSTEM
  EXCLUDE_FROM_ALL
  OVERRIDE_FIND_PACKAGE
)

# Disable benchmark tests, install, and warnings-as-errors (external dep)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

# Suppress warnings for external benchmark library (not our code)
# These are GCC/Clang flags, only needed on non-MSVC compilers
if(TARGET benchmark AND NOT MSVC)
  target_compile_options(benchmark PRIVATE
    -Wno-format-nonliteral
    -Wno-old-style-cast
    -Wno-shorten-64-to-32
    -Wno-float-equal
    -Wno-useless-cast
    -Wno-sign-conversion
  )
endif()

###############################################################################
# Benchmark Executable                                                        #
###############################################################################
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
  *.cpp
  *.hpp
  #"${CMAKE_SOURCE_DIR}/src/*.cpp"
  #"${CMAKE_SOURCE_DIR}/src/core/*.h"
)

add_executable(ropic-benchmarks
    ${SRC_FILES}
)

target_compile_features(ropic-benchmarks PRIVATE cxx_std_20)
target_link_libraries(ropic-benchmarks PRIVATE
    ropic
    benchmark::benchmark
    benchmark::benchmark_main
)

find_path(LIBPFM_INCLUDE_DIR NAMES perfmon/pfmlib.h)

# CHECK FOR LIBPFM4 (Required ONLY on Linux) ---
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT LIBPFM_INCLUDE_DIR)
        message(FATAL_ERROR 
            "\n\n"
            "------------------------------------------------------------------\n"
            " [!] ERROR: Missing 'libpfm4-dev' library.\n"
            "     This dependency is required for Google Benchmark.\n"
            " [>] Please run the following command in your terminal to install:\n"
            "     sudo apt update && sudo apt install -y libpfm4-dev git\n"
            "------------------------------------------------------------------\n"
        )
    endif()
endif()

###############################################################################
# Output Directory                                                            #
###############################################################################
set_target_properties(ropic-benchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

###############################################################################
# Print information                                                           #
###############################################################################
message(STATUS "================================================================")
message(STATUS "'${PROJECT_NAME}' in ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "----------------------------------------------------------------")
message(STATUS "Benchmark executable: ropic-benchmarks")
message(STATUS "================================================================")
