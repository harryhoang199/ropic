# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(ropic-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

###############################################################################
# FetchContent for Google Test                                               #
###############################################################################
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  #GIT_TAG 
  GIT_SHALLOW ON
  SYSTEM
  EXCLUDE_FROM_ALL
  OVERRIDE_FIND_PACKAGE
)
# Prevent GTest from overriding parent project's compiler/linker settings (MSVC)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Suppress warnings for external GoogleTest library (not our code)
# These are GCC/Clang flags, only needed on non-MSVC compilers
if(NOT MSVC)
  set(CONFIG_GTEST_COMPILE_OPTIONS 
      -Wno-double-promotion
      -Wno-useless-cast
  )
  if(TARGET gtest)
    target_compile_options(gtest PRIVATE ${CONFIG_GTEST_COMPILE_OPTIONS})
  endif()
  if(TARGET gtest_main)
    target_compile_options(gtest_main PRIVATE ${CONFIG_GTEST_COMPILE_OPTIONS})
  endif()
endif()

enable_testing()

###############################################################################
# Add executable and link libraries                                           #
###############################################################################
add_subdirectory(either)

###############################################################################
# Install Targets                                                             #
###############################################################################
# Note: Test executables are typically not installed, but this section is
# provided for completeness. Uncomment if you want to install test binaries.
#
# install(
#   TARGETS either-tests
#   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/tests
# )

###############################################################################
# Print information                                                           #
###############################################################################
message(STATUS "================================================================")
message(STATUS "'${PROJECT_NAME}' in ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "----------------------------------------------------------------")

message(STATUS "Targets:")
# Get and print all targets from the current directory
get_property(local_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
foreach(tgt IN LISTS local_targets)
  message(STATUS "  ${tgt}")
endforeach()

message(STATUS "Subdirectories:")
# Find subdirectories then get and print all targets from them
get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
foreach(subdir IN LISTS subdirs)
  message(STATUS "  ${subdir}")
endforeach()

