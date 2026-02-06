# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(ropic-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

###############################################################################
message(STATUS "================================================================")
message(STATUS "'${PROJECT_NAME}' in ${CMAKE_CURRENT_SOURCE_DIR}")

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

# set(GTEST_TARGETS gtest gtest_main gmock gmock_main)

# foreach(target ${GTEST_TARGETS})
#     if(TARGET ${target})
#         set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "")
#     endif()
# endforeach()

# Suppress warnings for external GoogleTest library (not our code)
# These are GCC/Clang flags, only needed on non-MSVC compilers
if(NOT MSVC)
  set(CONFIG_GTEST_COMPILE_OPTIONS
      -Wno-double-promotion
      -Wno-useless-cast
      -Wno-null-dereference
  )
  if(TARGET gtest)
    target_compile_options(gtest PRIVATE ${CONFIG_GTEST_COMPILE_OPTIONS})
  endif()
  if(TARGET gtest_main)
    target_compile_options(gtest_main PRIVATE ${CONFIG_GTEST_COMPILE_OPTIONS})
  endif()
endif()

# Disable LTO for GoogleTest targets to avoid link-time incompatibilities
# GoogleTest is compiled separately and may not be compatible with ThinLTO
# We must explicitly add -fno-lto to override the global -flto=thin flags
set(GTEST_TARGETS gtest gtest_main gmock gmock_main)
foreach(target ${GTEST_TARGETS})
  if(TARGET ${target})
    set_target_properties(${target} PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION OFF
      INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF
    )
    # Override global LTO flags with -fno-lto (Clang/GCC)
    if(NOT MSVC)
      target_compile_options(${target} PRIVATE -fno-lto)
      target_link_options(${target} PRIVATE -fno-lto)
    endif()
  endif()
endforeach()

###############################################################################
# Add executable and link libraries                                           #
###############################################################################
add_subdirectory(either)


###############################################################################
# Print information                                                           #
###############################################################################
# Function to recursively retrieve all targets in a directory and its subdirectories
function(get_all_targets_recursive _dir _out_var)
    # 1. Get targets defined directly in the current directory
    get_directory_property(_targets DIRECTORY "${_dir}" BUILDSYSTEM_TARGETS)
    
    # 2. Get list of subdirectories in the current directory
    get_directory_property(_subdirs DIRECTORY "${_dir}" SUBDIRECTORIES)

    # 3. Recursively iterate through each subdirectory
    foreach(_subdir ${_subdirs})
        get_all_targets_recursive("${_subdir}" _subdir_targets)
        list(APPEND _targets ${_subdir_targets})
    endforeach()

    # 4. Return the result list to the parent scope
    set(${_out_var} ${_targets} PARENT_SCOPE)
endfunction()

message(STATUS "----------------------------------------------------------------")
# Get all targets within the "tests" directory (recursive)
get_all_targets_recursive("${dir}" ALL_TEST_TARGETS)

message(STATUS "Found test targets: ${ALL_TEST_TARGETS}")

# Apply definitions only to EXECUTABLE targets found
foreach(tgt ${ALL_TEST_TARGETS})
    # Check the target type (EXECUTABLE, STATIC_LIBRARY, SHARED_LIBRARY, etc.)
    get_target_property(tgt_type ${tgt} TYPE)
    
    if (tgt_type STREQUAL "EXECUTABLE")
        # Apply the testing flag specifically to this target
        target_compile_definitions(${tgt} PRIVATE ROPIC_TESTING_MODE=1)
        message(STATUS "Applied ROPIC_TESTING_MODE to: ${tgt}")
    endif()
endforeach()

message(STATUS "----------------------------------------------------------------")
message(STATUS "Subdirectories:")
# Find subdirectories then get and print all targets from them
get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
foreach(subdir IN LISTS subdirs)
  message(STATUS "  ${subdir}")
endforeach()

