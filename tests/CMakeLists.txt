# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(ropic-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

###############################################################################
# FetchContent for Google Test                                               #
###############################################################################
find_package(GTest REQUIRED)
# Prevent GTest from overriding parent project's compiler/linker settings (MSVC)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

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

