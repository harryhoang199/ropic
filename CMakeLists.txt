# CMakeLists.txt
cmake_minimum_required(VERSION 3.28)

# Read version from VERSION file
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" ROPIC_VERSION)
string(STRIP "${ROPIC_VERSION}" ROPIC_VERSION)

project(ropic
  VERSION ${ROPIC_VERSION}
  DESCRIPTION "Railway Oriented Programming library for C++20"
  LANGUAGES CXX
)

# cmake_policy(SET CMP0167 NEW)


#  CHECK FOR GIT (Required on ALL platforms for FetchContent) ---
find_package(Git QUIET)
if(NOT GIT_FOUND)
    message(FATAL_ERROR 
        "\n\n"
        "------------------------------------------------------------------\n"
        " [!] ERROR: 'git' is not found in your PATH.\n"
        "     CMake needs git to download Google Test/Benchmark.\n\n"
        " [>] Windows: Install Git for Windows.\n"
        " [>] Linux:   sudo apt install git\n"
        "------------------------------------------------------------------\n"
    )
endif()

# Set output directories (multi-config generators like VS append Debug/Release automatically)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# INSTALLATION CONFIGURATION 
# Check if the user has NOT specified a custom CMAKE_INSTALL_PREFIX.
# If they provided a prefix (e.g., -DCMAKE_INSTALL_PREFIX=/opt/myapp), we respect it.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Check for "Developer Mode" conditions:
    # 1. This is the top-level project (not included via add_subdirectory).
    # 2. We are NOT running in a CI environment (Github Actions, GitLab CI, etc.).
    #    CI systems usually set the "CI" environment variable.
    if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR AND NOT DEFINED ENV{CI})

        # --- DEVELOPMENT MODE ---
        # Automatically set the install prefix to a local "install" folder inside the build directory.
        # This allows developers to run 'cmake --install build' without sudo/admin rights.
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install prefix for dev" FORCE)

        message(STATUS "Default install path set to: ${CMAKE_INSTALL_PREFIX}")
        
    else()
        # --- USER / SYSTEM / CI MODE ---
        # Keep the default system path (e.g., /usr/local on Linux, Program Files on Windows).
        # Or respect the parent project's configuration.
        message(STATUS "Default install path: ${CMAKE_INSTALL_PREFIX}")
    endif()

    message(STATUS "Use '-DCMAKE_INSTALL_PREFIX=...' to override this.")
endif()

###############################################################################
# Configure Build Process                                                     #
###############################################################################
if (MSVC)
  add_compile_options(
    # === C+  Standard ===
    /std:c++20                  # Enable C++20 standard features and language rules

    # === Build Performance ===
    /FS                         # Allow parallel PDB (Program Database) file access for multi-process builds
    /MP                         # Enable multi-processor compilation (parallel build across translation units)

    # === Warning Levels ===
    /W4                         # Enable warning level 4 (high). Covers most of -Wall -Wextra from GCC.
                                # Note: /Wall enables ALL warnings including noisy ones from system headers
    # /WX                       # Treat all warnings as errors. Uncomment for strict builds

    # === Security Hardening ===
    /GS                         # Enable buffer security checks (stack canaries). GCC equivalent: -fstack-protector-strong
    /sdl                        # Enable additional Security Development Lifecycle checks:
                                # - Stricter type checking, pointer validation, format string checks
                                # GCC equivalent: combination of -Wformat=2 and runtime checks

    # === Exception Handling ===
    /EHsc                       # Enable C+  exception handling with extern "C" defaults to nothrow
                                # 's' = synchronous exceptions only, 'c' = extern "C" functions don't throw

    # === Standards Conformance ===
    /permissive-                # Enforce strict C+  standard conformance, reject non-standard extensions
                                # GCC equivalent: -Wpedantic
    /Zc:__cplusplus             # Report correct __cplusplus macro value (MSVC defaults to 199711L otherwise)
    /Zc:inline                  # Remove unreferenced functions/data from COMDAT sections (better optimization)
    /Zc:preprocessor            # Use new conforming C++20 preprocessor (required for __VA_OPT__, etc.)

    # === External Header Handling ===
    /external:anglebrackets     # Treat all #include <...> headers as external (system) headers
    /external:W0                # Disable all warnings for external headers (reduces noise from third-party libs)

    # === Workarounds ===
    /d2FH4-                     # Disable FH4 exception handling for better IntelliSense compatibility
                                # Workaround for VS2019  IntelliSense issues with C++20 and STL

    # === Additional Warnings (Level 4 Specific) ===
    # Shadow warnings: warn when local variable shadows another variable
    /w14456                     # Local variable shadows a local variable in outer scope
    /w14457                     # Local variable shadows a function parameter
    /w14458                     # Class member declaration shadows a class member
    /w14459                     # Local variable shadows a global declaration

    # Object-oriented warnings
    /w14263                     # Member function doesn't override any base class virtual function
    /w14265                     # Class has virtual functions but destructor is not virtual
    /we4289                     # Loop control variable declared in for-loop used outside loop scope (ERROR)

    # Conversion warnings: potential data loss or unexpected behavior
    /w14242                     # Conversion from 'type1' to 'type2': possible loss of data (narrowing)
    /w14254                     # Conversion from 'type1:bitfield' to 'type2': possible loss of data
    /w14287                     # Unsigned/negative constant mismatch in conditional expression
    /w14296                     # Expression is always true/false (comparison of unsigned with negative)
    /w14311                     # Pointer truncation from 'type1' to 'type2' (64-bit portability)
    /w14826                     # Conversion from 'type1' to 'type2' is sign-extended (may cause issues)

    # Expression and operator warnings
    /w14545                     # Expression before comma evaluates to function missing argument list
    /w14546                     # Function call before comma missing argument list
    /w14547                     # Operator before comma has no effect; expected side-effect operator
    /w14549                     # Operator before comma has no effect; did you mean another operator?
    /w14555                     # Expression has no effect; expected expression with side-effect

    # Control flow warnings
    /w15262                     # Implicit fall-through in switch case (add [[fallthrough]] if intentional)

    # Miscellaneous warnings
    /w14619                     # Pragma warning: warning number 'X' does not exist
    /w14640                     # Thread-unsafe static local variable initialization (C++11 fixes this)
    /w14905                     # Wide string literal cast to 'LPSTR' (unsafe narrowing)
    /w14906                     # String literal cast to 'LPWSTR' (unsafe widening)
    /w14928                     # Illegal copy-initialization; more than one user-defined conversion applied
  )

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    # === C+  Standard ===
    -std=c++20                  # Enable C++20 standard features and language rules

    # === Core Warning Flags ===
    -Wall                       # Enable most common warnings: unused variables, uninitialized values,
                                # missing returns, suspicious comparisons, format string issues, etc.
    -Wextra                     # Enable extra warnings not covered by -Wall: unused parameters,
                                # missing field initializers, sign comparisons, etc.
    -Wpedantic                  # Enforce strict ISO C+  compliance, warn on compiler extensions
    # -Werror                   # Treat all warnings as errors. Uncomment for strict builds

    # === Type Conversion Warnings ===
    -Wconversion                # Warn on implicit type conversions that may alter values
                                # (e.g., double to int, long to short)
    -Wsign-conversion           # Warn on implicit sign conversions (e.g., signed to unsigned)
    -Wdouble-promotion          # Warn when float is implicitly promoted to double (performance issue
                                # on some architectures; often unintentional in printf-style calls)

    # === Object-Oriented Warnings ===
    -Wshadow                    # Warn when a local variable or parameter shadows another variable
    -Wnon-virtual-dtor          # Warn when a class with virtual functions has a non-virtual destructor
                                # (potential memory leak when deleting through base pointer)
    -Wold-style-cast            # Warn on C-style casts; prefer static_cast, dynamic_cast, etc.
    -Woverloaded-virtual        # Warn when a derived class function hides a virtual function from base
                                # (instead of overriding it - usually a bug)

    # === Null Pointer and Memory Safety ===
    -Wnull-dereference          # Warn when compiler detects paths that dereference null pointers
    -Wcast-align                # Warn when pointer cast increases required alignment (e.g., char* to int*)
                                # Can cause crashes on architectures requiring strict alignment
    -Wcast-qual                 # Warn when casting removes type qualifiers (e.g., const_cast away const)

    # === Format String Security ===
    -Wformat=2                  # Enable strict format string checking for printf/scanf family:
                                # - Check format specifiers match argument types
                                # - Warn on non-literal format strings (security risk)
                                # - Warn on missing format arguments

    # === Control Flow Warnings ===
    -Wimplicit-fallthrough      # Warn when switch case falls through without [[fallthrough]] annotation

    # === GCC-Specific Warnings ===
    -Wduplicated-cond           # Warn when if-else chain has duplicated conditions (likely copy-paste bug)
    -Wduplicated-branches       # Warn when if-else branches have identical code (likely copy-paste bug)
    -Wlogical-op                # Warn on suspicious logical operations (e.g., always true/false expressions)
    -Wuseless-cast              # Warn on useless casts (casting to the same type)

    # === Security Hardening ===
    -fstack-protector-strong    # Insert stack canaries for functions with local arrays or address-taken
                                # variables. Detects stack buffer overflows at runtime
    -fno-common                 # Prevent multiple definitions of global variables (stricter linking)
                                # Default in C+  but good to be explicit

    # === Output Formatting ===
    -fdiagnostics-color=always  # Force colored diagnostic output even when piped (useful in CI/IDE)
  )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  add_compile_options(
    # === C+  Standard ===
    -std=c++20                  # Enable C++20 standard features and language rules

    # === Core Warning Flags (shared with GCC) ===
    -Wall                       # Enable most common warnings: unused variables, uninitialized values,
                                # missing returns, suspicious comparisons, format string issues, etc.
    -Wextra                     # Enable extra warnings not covered by -Wall: unused parameters,
                                # missing field initializers, sign comparisons, etc.
    -Wpedantic                  # Enforce strict ISO C+  compliance, warn on compiler extensions
    # -Werror                   # Treat all warnings as errors. Uncomment for strict builds

    # === Type Conversion Warnings ===
    -Wconversion                # Warn on implicit type conversions that may alter values
    -Wsign-conversion           # Warn on implicit sign conversions (e.g., signed to unsigned)
    -Wdouble-promotion          # Warn when float is implicitly promoted to double

    # === Object-Oriented Warnings ===
    -Wshadow-all                # Comprehensive shadow warnings (Clang-enhanced):
                                # - Local variables shadowing locals, parameters, or globals
                                # - Class members shadowing base class members (not in GCC's -Wshadow)
    -Wnon-virtual-dtor          # Warn when polymorphic class has non-virtual destructor
    -Wold-style-cast            # Warn on C-style casts; prefer static_cast, dynamic_cast, etc.
    -Woverloaded-virtual        # Warn when derived class function hides base class virtual function

    # === Null Pointer and Memory Safety ===
    -Wnull-dereference          # Warn when compiler detects paths that dereference null pointers
    -Wcast-align                # Warn when pointer cast increases required alignment
    -Wcast-qual                 # Warn when casting removes type qualifiers (e.g., const)

    # === Format String Security ===
    -Wformat=2                  # Enable strict format string checking (security-critical)

    # === Control Flow Warnings ===
    -Wimplicit-fallthrough      # Warn on switch case fall-through without [[fallthrough]]

    # === Clang-Specific Warnings ===
    -Wthread-safety             # Static analysis for thread safety (Google-developed):
                                # - Warns on unlocked access to guarded data
                                # - Requires annotations like GUARDED_BY(), REQUIRES(), etc.
                                # - Zero runtime overhead (compile-time only)
    -Wdocumentation             # Validate documentation comments (Doxygen-style):
                                # - Check @param names match actual parameters
                                # - Verify @return exists for non-void functions
    -Wcomma                     # Warn on suspicious uses of comma operator (often bugs)
    -Wloop-analysis             # Detect loop issues: infinite loops, loop variable issues
    -Wmove                      # Warn on pessimizing std::move usage:
                                # - Moving from const objects (no effect)
                                # - Moving when copy elision would apply (prevents RVO/NRVO)
    -Wrange-loop-analysis       # Warn on range-based for loop issues:
                                # - Copying large objects when reference would suffice
                                # - Loop variable type mismatches
    -Wunreachable-code-aggressive # More aggressive unreachable code detection than default

    # === Compatibility ===
    -Wno-unknown-warning-option # Suppress warnings about unrecognized warning flags
                                # (useful for forward compatibility across Clang versions)

    # === Security Hardening ===
    -fstack-protector-strong    # Insert stack canaries for overflow detection
    -fno-common                 # Prevent multiple global variable definitions

    # === Output Formatting ===
    -fcolor-diagnostics         # Enable colored diagnostic output (Clang's equivalent of GCC flag)
  )

endif()

###############################################################################
# Configuration-Specific Options (Debug vs Release)                           #
###############################################################################
# These options differ between Debug and Release builds to optimize for
# either development/debugging or production performance.

if(MSVC)
  #############################################################################
  # MSVC: Debug Configuration
  #############################################################################
  add_compile_options(
    # === Debug: Optimization ===
    $<$<CONFIG:Debug>:/Od>      # Disable all optimizations for accurate debugging
                                # Variables won't be optimized away, stepping works correctly

    # === Debug: Debug Information ===
    $<$<CONFIG:Debug>:/Zi>      # Generate complete debug info in separate PDB file
                                # Best for debugging; allows Edit and Continue in VS

    # === Debug: Runtime Checks ===
    # Choose ONE: /RTC1 (simple, always works) OR /fsanitize=address (powerful, requires setup)
    $<$<CONFIG:Debug>:/RTC1>    # Runtime error checks:
                                # - /RTCs: Stack frame runtime checking
                                # - /RTCu: Report use of uninitialized variables
                                # Simple and reliable, no extra setup needed

    # === Debug: Sanitizers (VS2019 16.9+) ===
    # $<$<CONFIG:Debug>:/fsanitize=address>  # AddressSanitizer: more powerful but requires:
                                             # - ASan runtime DLLs in PATH
                                             # - Disable incremental linking (/INCREMENTAL:NO)
                                             # - Use /Zi instead of /ZI (no Edit and Continue)
                                             # Uncomment and fix linker settings if needed

    #############################################################################
    # MSVC: Release Configuration
    #############################################################################
    # === Release: Optimization ===
    $<$<CONFIG:Release>:/O2>    # Maximize speed (equivalent to /Og /Oi /Ot /Oy /Ob2 /GF /Gy)
                                # Best balance of speed optimizations

    # === Release: Debug Information ===
    $<$<CONFIG:Release>:/Zi>    # Still generate PDB for crash dump analysis
                                # Release builds should be debuggable for production issues

    # === Release: Link-Time Optimization ===
    $<$<CONFIG:Release>:/GL>    # Enable Whole Program Optimization (WPO)
                                # Delays code generation to link time for better optimization
                                # Requires /LTCG linker flag (added below)

    # === Release: Additional Optimizations ===
    $<$<CONFIG:Release>:/Gw>    # Optimize global data (place each global in separate COMDAT)
                                # Enables linker to remove unused globals
    $<$<CONFIG:Release>:/Gy>    # Enable function-level linking (separate COMDATs)
                                # Allows linker to remove unused functions
  )

  # Linker flags
  add_link_options(
    #############################################################################
    # Security Hardening (all configurations)
    #############################################################################
    /DYNAMICBASE                        # Enable ASLR (Address Space Layout Randomization)
                                        # Randomizes base address at load time (default on, explicit is clearer)
    /HIGHENTROPYVA                      # Enable high-entropy 64-bit ASLR
                                        # Uses full 64-bit address space for better randomization
    /NXCOMPAT                           # Mark executable as DEP (Data Execution Prevention) compatible
                                        # Prevents code execution from data pages (default on, explicit)
    /CETCOMPAT                          # Enable Intel CET (Control-flow Enforcement Technology)
                                        # Hardware-assisted control flow integrity (requires CET-capable CPU)
    # /GUARD:CF                         # Control Flow Guard - requires /guard:cf compiler flag
                                        # Validates indirect call targets at runtime
                                        # Uncomment after adding /guard:cf to compile options

    #############################################################################
    # Debug Configuration
    #############################################################################
    $<$<CONFIG:Debug>:/DEBUG:FULL>      # Generate complete debug information
                                        # Required for debugging; creates .pdb file
    $<$<CONFIG:Debug>:/INCREMENTAL>     # Enable incremental linking for faster rebuilds
                                        # Only re-links changed object files (default for Debug)

    #############################################################################
    # Release Configuration
    #############################################################################
    $<$<CONFIG:Release>:/DEBUG:FULL>    # Still generate full PDB for crash dump analysis
                                        # Essential for debugging production issues
    $<$<CONFIG:Release>:/LTCG>          # Link-Time Code Generation (required for /GL)
                                        # Performs whole-program optimization at link time
    $<$<CONFIG:Release>:/OPT:REF>       # Remove unreferenced functions and data
                                        # Dead code/data elimination
    $<$<CONFIG:Release>:/OPT:ICF>       # Identical COMDAT Folding
                                        # Merge identical functions/data to reduce size
    $<$<CONFIG:Release>:/INCREMENTAL:NO> # Disable incremental linking for Release
                                         # Required for /LTCG and /OPT optimizations
  )

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  #############################################################################
  # GCC: Debug Configuration
  #############################################################################
  add_compile_options(
    # === Debug: Optimization ===
    $<$<CONFIG:Debug>:-O0>      # No optimization - code maps directly to source
                                # Required for accurate debugging and variable inspection

    # === Debug: Debug Information ===
    $<$<CONFIG:Debug>:-g3>      # Maximum debug information level:
                                # - Includes macro definitions
                                # - Extra info for debugger expressions
                                # Larger binaries but best debugging experience

    # === Debug: Sanitizers ===
    # AddressSanitizer: Detects memory errors at runtime
    $<$<CONFIG:Debug>:-fsanitize=address>   # Buffer overflows, use-after-free, memory leaks
    $<$<CONFIG:Debug>:-fsanitize=undefined> # Undefined Behavior Sanitizer:
                                            # - Signed integer overflow
                                            # - Null pointer dereference
                                            # - Invalid shifts, division by zero
                                            # - Misaligned pointer access
    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>  # Keep frame pointers for better stack traces
                                                # Required for accurate sanitizer reports

    #############################################################################
    # GCC: Release Configuration
    #############################################################################
    # === Release: Optimization ===
    $<$<CONFIG:Release>:-O3>    # Aggressive optimizations including:
                                # - Inline functions, loop unrolling
                                # - Vectorization (SIMD)
                                # - Expensive optimizations enabled
                                # Note: -O2 is safer; -O3 may increase code size

    # === Release: Debug Information ===
    $<$<CONFIG:Release>:-g1>    # Minimal debug info (line numbers, function names)
                                # Enough for stack traces in crash dumps
                                # Much smaller than -g or -g3

    # === Release: Link-Time Optimization ===
    $<$<CONFIG:Release>:-flto=auto>     # Enable Link-Time Optimization with automatic parallelism
                                        # Performs whole-program optimization at link time
                                        # - Cross-file inlining
                                        # - Dead code elimination
                                        # - Interprocedural optimizations
    $<$<CONFIG:Release>:-fdata-sections>    # Place each data item in separate section
                                            # Enables --gc-sections to remove unused data
    $<$<CONFIG:Release>:-ffunction-sections> # Place each function in separate section
                                             # Enables --gc-sections to remove unused functions
  )

  # Linker flags
  add_link_options(
    #############################################################################
    # Security Hardening (all configurations)
    #############################################################################
    -Wl,-z,relro                        # Partial RELRO: Make GOT (Global Offset Table) read-only
                                        # after dynamic linker resolves symbols
    -Wl,-z,now                          # Full RELRO: Resolve all symbols at load time
                                        # Combined with relro, makes GOT completely read-only
                                        # Prevents GOT overwrite attacks
    -Wl,-z,noexecstack                  # Mark stack as non-executable (NX bit)
                                        # Prevents stack-based code execution attacks
    -Wl,-z,separate-code                # Separate code and data into different pages
                                        # Helps prevent code page corruption
    # -pie                              # Position Independent Executable (for ASLR)
                                        # Uncomment if building executable (not needed for libs)
                                        # Note: May conflict with some debugging setups

    #############################################################################
    # Debug Configuration
    #############################################################################
    $<$<CONFIG:Debug>:-fsanitize=address>   # Link AddressSanitizer runtime
    $<$<CONFIG:Debug>:-fsanitize=undefined> # Link UndefinedBehaviorSanitizer runtime

    #############################################################################
    # Release Configuration
    #############################################################################
    $<$<CONFIG:Release>:-flto=auto>         # Link-Time Optimization (match compiler flag)
    $<$<CONFIG:Release>:-Wl,--gc-sections>  # Remove unused sections
                                            # Works with -ffunction-sections and -fdata-sections
    $<$<CONFIG:Release>:-Wl,--as-needed>    # Only link libraries that resolve undefined symbols
                                            # Reduces unnecessary library dependencies
    $<$<CONFIG:Release>:-Wl,-O1>            # Enable linker optimizations
                                            # Optimizes hash tables and other linker structures
  )

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  #############################################################################
  # Clang: Debug Configuration
  #############################################################################
  add_compile_options(
    $<$<CONFIG:Debug>:-Rpass=coro-elide>      # 
    # === Debug: Optimization ===
    $<$<CONFIG:Debug>:-O0>      # No optimization for accurate debugging

    # === Debug: Debug Information ===
    $<$<CONFIG:Debug>:-g3>      # Maximum debug info including macro definitions
    $<$<CONFIG:Debug>:-fstandalone-debug>   # Emit full debug info for all types
                                            # (Clang normally omits debug info for types
                                            # defined in system headers)

    # === Debug: Sanitizers ===
    $<$<CONFIG:Debug>:-fsanitize=address>   # AddressSanitizer for memory errors
    $<$<CONFIG:Debug>:-fsanitize=undefined> # UndefinedBehaviorSanitizer
    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>  # Keep frame pointers for stack traces

    #############################################################################
    # Clang: Release Configuration
    #############################################################################
    # === Release: Optimization ===
    $<$<CONFIG:Release>:-O3>    # Maximum optimization level

    # === Release: Debug Information ===
    $<$<CONFIG:Release>:-gline-tables-only> # Minimal debug info: only line numbers
                                            # Smallest overhead, enough for stack traces
                                            # Clang-specific; more compact than -g1

    # === Release: Link-Time Optimization ===
    $<$<CONFIG:Release>:-flto=thin>     # ThinLTO: Clang's scalable LTO implementation
                                        # - Faster link times than full LTO
                                        # - Parallel optimization
                                        # - Nearly as good optimization as full LTO
                                        # Use -flto=full for maximum optimization (slower links)
    $<$<CONFIG:Release>:-fdata-sections>
    $<$<CONFIG:Release>:-ffunction-sections>
  )

  # Linker flags
  add_link_options(
    #############################################################################
    # Security Hardening (all configurations)
    # Note: Some flags differ between Linux (ld) and macOS (ld64)
    #############################################################################
    $<$<NOT:$<PLATFORM_ID:Darwin>>:-Wl,-z,relro>      # Partial RELRO (Linux/BSD only)
    $<$<NOT:$<PLATFORM_ID:Darwin>>:-Wl,-z,now>        # Full RELRO - resolve all symbols at load
    $<$<NOT:$<PLATFORM_ID:Darwin>>:-Wl,-z,noexecstack> # Non-executable stack (Linux/BSD)
    $<$<NOT:$<PLATFORM_ID:Darwin>>:-Wl,-z,separate-code> # Separate code/data pages
    # macOS: Security features are largely handled by the OS and Hardened Runtime

    #############################################################################
    # Debug Configuration
    #############################################################################
    $<$<CONFIG:Debug>:-fsanitize=address>   # Link AddressSanitizer runtime
    $<$<CONFIG:Debug>:-fsanitize=undefined> # Link UndefinedBehaviorSanitizer runtime

    #############################################################################
    # Release Configuration
    #############################################################################
    $<$<CONFIG:Release>:-flto=thin>             # ThinLTO (match compiler flag)

    # Platform-specific dead code elimination
    $<$<AND:$<CONFIG:Release>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--gc-sections>  # Linux: Remove unused sections
    $<$<AND:$<CONFIG:Release>,$<PLATFORM_ID:Darwin>>:-Wl,-dead_strip>           # macOS: Equivalent to --gc-sections

    # Linux-specific optimizations
    $<$<AND:$<CONFIG:Release>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,--as-needed>    # Only link needed libraries
    $<$<AND:$<CONFIG:Release>,$<NOT:$<PLATFORM_ID:Darwin>>>:-Wl,-O1>            # Linker optimizations
  )

endif()


###############################################################################
# Generate Version Header and ropic Header                                                     #
###############################################################################
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ropic/version.hpp
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ropic.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/ropic/ropic
  @ONLY
)

###############################################################################
# Header-Only Library Target                                                  #
###############################################################################

# Collect all header files
file(GLOB_RECURSE ROPIC_HEADERS CONFIGURE_DEPENDS
  src/*.hpp
  src/*.inl
)

# Create INTERFACE library (header-only)
add_library(ropic INTERFACE)
add_library(ropic::ropic ALIAS ropic)

target_compile_features(ropic INTERFACE cxx_std_20)

set(ROPIC_INSTALL_SHORTCUT_INCLUDEDIR shortcut_include)
target_include_directories(ropic INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<INSTALL_INTERFACE:${ROPIC_INSTALL_SHORTCUT_INCLUDEDIR}>
)

###############################################################################
# Options                                                                     #
###############################################################################
option(ROPIC_BUILD_EXAMPLES "Build example executable" OFF)
option(ROPIC_BUILD_TESTING "Build tests" OFF)
option(ROPIC_BUILD_BENCHMARKS "Build performance benchmarks" OFF)

###############################################################################
# Activate testing if ROPIC_BUILD_TESTING is ON                                     #
###############################################################################
if(ROPIC_BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

###############################################################################
# Activate benchmarks if ROPIC_BUILD_BENCHMARKS is ON                         #
###############################################################################
if(ROPIC_BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

###############################################################################
# Example Executable (optional)                                               #
###############################################################################
if(ROPIC_BUILD_EXAMPLES)
  add_executable(ropic_examples
    main.cpp
  )

  target_compile_features(ropic_examples PRIVATE cxx_std_20)
  target_link_libraries(ropic_examples PRIVATE ropic)
endif()


###############################################################################
# Add necessary macros for example executable                                 #
###############################################################################
if(ROPIC_BUILD_EXAMPLES)
  target_compile_definitions(ropic_examples PRIVATE
    $<$<CONFIG:Debug>:_DEBUG> #Macro _DEBUG
    $<$<CONFIG:Release>:NDEBUG> #Macro NDEBUG for disabling assert()
    $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601> #Macro _WIN32_WINNT
  )
endif()

###############################################################################
# Install Targets                                                             #
###############################################################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install header files preserving directory structure
install(
  DIRECTORY src/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ropic
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.inl"
)

# Install generated version and ropic header
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/ropic/version.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ropic
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/ropic/ropic
  DESTINATION ${ROPIC_INSTALL_SHORTCUT_INCLUDEDIR}
)

# Install the library target
install(
  TARGETS ropic
  EXPORT ropicTargets
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the export set
install(
  EXPORT ropicTargets
  FILE ropicTargets.cmake
  NAMESPACE ropic::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ropic
)

# Generate and install package config files
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ropicConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ropicConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ropic
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/ropicConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ropicConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ropicConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ropic
)

# Optionally install example executable
if(ROPIC_BUILD_EXAMPLES)
  install(
    TARGETS ropic_examples
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()


###############################################################################
# Print information
###############################################################################
message(STATUS "================================================================")
message(STATUS "'${PROJECT_NAME}' v${PROJECT_VERSION} in ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "----------------------------------------------------------------")

message(STATUS "CMake system prefix paths:")
foreach(path ${CMAKE_SYSTEM_PREFIX_PATH})
  message(STATUS "  ${path}")
endforeach()
message(STATUS "CMake prefix paths:")
foreach(path ${CMAKE_PREFIX_PATH})
  message(STATUS "  ${path}")
endforeach()

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

message(STATUS "Header files:")
foreach(header_file ${ROPIC_HEADERS})
  message(STATUS "  ${header_file}")
endforeach()

message(STATUS "Include directories:")
get_target_property(INCLUDE_DIRS ropic INTERFACE_INCLUDE_DIRECTORIES)
foreach(include_dir ${INCLUDE_DIRS})
  message(STATUS "  ${include_dir}")
endforeach()
message(STATUS "================================================================")