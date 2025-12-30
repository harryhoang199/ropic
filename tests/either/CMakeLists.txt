# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(either-tests LANGUAGES CXX)

###############################################################################
# Detect and print source files                                               #
# TODO: Use a more robust method to detect source files                       #
###############################################################################
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
  *.cpp
  *.h
  #"${CMAKE_SOURCE_DIR}/src/*.cpp"
  #"${CMAKE_SOURCE_DIR}/src/core/*.h"
)

###############################################################################
# Add executable and link libraries                                           #
###############################################################################
add_executable(either-tests 
  ${SRC_FILES}
)

target_compile_features(either-tests PRIVATE cxx_std_20)
target_link_libraries(either-tests PRIVATE
  ropic
  GTest::gtest_main
)

###############################################################################
# Enable Google Test discovery                                                #
# This will automatically discover tests in the either-tests executable      #
# and run them when using `ctest` or `make test`                              #
###############################################################################
# include(GoogleTest)
# gtest_discover_tests(either-tests  
  # EXTRA_ARGS --gtest_color=yes
# )

###############################################################################
# Flags for sanitizer                                                         #
###############################################################################
if (MSVC)
    # MSVC uses /fsanitize=address since Visual Studio 2019 16.9+
    #target_compile_options(either-tests PRIVATE /fsanitize=address /Zi /Od)
else()
    # GCC / Clang
    target_compile_options(either-tests PRIVATE -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -g)
    target_link_options(either-tests PRIVATE -fsanitize=address -fsanitize=leak)
endif()

###############################################################################
# Add necessary macros                                                        #
###############################################################################
target_compile_definitions(either-tests PRIVATE
  $<$<CONFIG:Debug>:_DEBUG> #Macro _DEBUG
  $<$<CONFIG:Release>:NDEBUG> #Macro NDEBUG for disabling assert() 
  $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601> #Macro _WIN32_WINNT
)

###############################################################################
# Install Targets                                                             #
###############################################################################
# Note: Test executables are typically not installed, but this section is
# provided for completeness. Uncomment if you want to install test binaries.
#
# include(GNUInstallDirs)
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

message(STATUS "Source files:")
foreach(src_file ${SRC_FILES})
  message(STATUS "  ${src_file}")
endforeach()

message(STATUS "Targets:")
# Get and print all targets from the current directory
get_property(local_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
foreach(tgt IN LISTS local_targets)
  message(STATUS "  ${tgt}")
endforeach()

message(STATUS "Link libraries:")
get_target_property(LINK_LIBS either-tests LINK_LIBRARIES)
foreach(lib ${LINK_LIBS})
  message(STATUS "  ${lib}")
endforeach()
