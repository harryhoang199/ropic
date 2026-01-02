# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(ropic-examples LANGUAGES CXX)

###############################################################################
# Detect and collect source files                                             #
###############################################################################
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
  *.cpp
  *.hpp
)

###############################################################################
# Add executable and link libraries                                           #
###############################################################################
add_executable(ropic-examples
  ${SRC_FILES}
)

target_compile_features(ropic-examples PRIVATE cxx_std_20)
target_link_libraries(ropic-examples PRIVATE
  ropic
)

###############################################################################
# Add necessary macros                                                        #
###############################################################################
target_compile_definitions(ropic-examples PRIVATE
  $<$<CONFIG:Debug>:_DEBUG> #Macro _DEBUG
  $<$<CONFIG:Release>:NDEBUG> #Macro NDEBUG for disabling assert()
  $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601> #Macro _WIN32_WINNT
)

###############################################################################
# Output Directory                                                            #
###############################################################################
set_target_properties(ropic-examples PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

###############################################################################
# Install Targets                                                             #
###############################################################################
include(GNUInstallDirs)
install(
  TARGETS ropic-examples
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

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
get_target_property(LINK_LIBS ropic-examples LINK_LIBRARIES)
foreach(lib ${LINK_LIBS})
  message(STATUS "  ${lib}")
endforeach()
message(STATUS "================================================================")
