cmake_minimum_required(VERSION 3.14)

# Define the project and its version (derived from vrpc/adapter.hpp)
project(vrpc-hpp VERSION 3.1.0 LANGUAGES CXX)

# ==============================================================================
# HOW TO CONSUME THIS LIBRARY (Instructions for End-Users)
# ==============================================================================
# You have three ways to use this library in your own project:
#
# 1. Via FetchContent (Recommended):
#    include(FetchContent)
#    FetchContent_Declare(vrpc GIT_REPOSITORY https://github.com/heisenware/vrpc-hpp.git GIT_TAG v3.1.0)
#    FetchContent_MakeAvailable(vrpc)
#    target_link_libraries(my_app PRIVATE vrpc::vrpc)
#
# 2. Via Submodule:
#    add_subdirectory(path/to/vrpc-hpp)
#    target_link_libraries(my_app PRIVATE vrpc::vrpc)
#
# 3. Via System Install:
#    find_package(vrpc REQUIRED)
#    target_link_libraries(my_app PRIVATE vrpc::vrpc)
# ==============================================================================


# ------------------------------------------------------------------------------
# 1. Target Definition
# ------------------------------------------------------------------------------
# vrpc-hpp is a header-only library, which in CMake is called an "INTERFACE" library.
add_library(vrpc INTERFACE)

# Create an alias target (vrpc::vrpc) so the library can be consumed the same way
# regardless of whether it's pulled in via add_subdirectory() or find_package().
add_library(vrpc::vrpc ALIAS vrpc)

# vrpc-hpp requires at least C++14. This forces any consuming application to
# compile with at least -std=c++14.
target_compile_features(vrpc INTERFACE cxx_std_14)


# ------------------------------------------------------------------------------
# 2. Base Include Directories
# ------------------------------------------------------------------------------
# Tell CMake where the core library headers are located.
# - BUILD_INTERFACE: Used when compiling directly from the source tree.
# - INSTALL_INTERFACE: Used when the user installs the library to their system.
target_include_directories(vrpc INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)


# ------------------------------------------------------------------------------
# 3. Boost Dependency Resolution
# ------------------------------------------------------------------------------
# We bundle a minimal version of Boost for convenience. However, to avoid ODR
# (One Definition Rule) violations and ABI conflicts, we attempt to use the
# user's system-installed Boost first. Users can force the bundled version via
# the VRPC_USE_BUNDLED_BOOST option.

option(VRPC_USE_BUNDLED_BOOST "Force vrpc-hpp to use its internal bundled Boost headers" OFF)

if(NOT VRPC_USE_BUNDLED_BOOST)
    # Attempt to find the system Boost. We search for the specific compiled components
    # that vrpc relies on under the hood (system, filesystem, atomic).
    find_package(Boost COMPONENTS system filesystem atomic QUIET)
endif()

if(Boost_FOUND AND NOT VRPC_USE_BUNDLED_BOOST)
    message(STATUS "vrpc-hpp: Using system-installed Boost ${Boost_VERSION}")

    # Link against the system Boost targets.
    # Boost::boost is the header-only target, the rest are compiled libraries.
    target_link_libraries(vrpc INTERFACE
        Boost::boost
        Boost::system
        Boost::filesystem
        Boost::atomic
    )
else()
    message(STATUS "vrpc-hpp: Using internal bundled Boost headers")

    # Expose the bundled boost directory so <boost/...> includes resolve correctly.
    # Note: Because this is an INTERFACE target, any user linking against vrpc::vrpc
    # automatically inherits this include path.
    target_include_directories(vrpc INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/vrpc/boost>
        $<INSTALL_INTERFACE:include/vrpc/boost>
    )
endif()


# ------------------------------------------------------------------------------
# 4. Optional Library Features (Macros)
# ------------------------------------------------------------------------------

# Feature: TLS / SSL support (VRPC_USE_TLS)
option(VRPC_WITH_TLS "Enable TLS/SSL support via OpenSSL" OFF)
if(VRPC_WITH_TLS)
    find_package(OpenSSL REQUIRED)

    # Define the macro for the C++ code
    target_compile_definitions(vrpc INTERFACE VRPC_USE_TLS)

    # Automatically link OpenSSL for any consumer of vrpc::vrpc
    target_link_libraries(vrpc INTERFACE OpenSSL::SSL OpenSSL::Crypto)
    message(STATUS "vrpc-hpp: TLS support ENABLED")
endif()

# Feature: Dynamic Plugin Loading (VRPC_WITH_DL)
option(VRPC_WITH_DL "Enable dynamic plugin loading support" OFF)
if(VRPC_WITH_DL AND NOT WIN32)
    target_compile_definitions(vrpc INTERFACE VRPC_WITH_DL)

    # On Linux/macOS, we must link against libdl
    target_link_libraries(vrpc INTERFACE ${CMAKE_DL_LIBS})
    message(STATUS "vrpc-hpp: Dynamic loading (DL) support ENABLED")
endif()

# Feature: Debug logging (VRPC_DEBUG)
option(VRPC_ENABLE_DEBUG_LOGGING "Enable vrpc internal debug logging" OFF)
if(VRPC_ENABLE_DEBUG_LOGGING)
    target_compile_definitions(vrpc INTERFACE VRPC_DEBUG)
    message(STATUS "vrpc-hpp: Debug logging ENABLED")
endif()

# Requirement: Threads
# Boost.Asio requires threading support to function properly on Unix systems.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(vrpc INTERFACE Threads::Threads)


# ------------------------------------------------------------------------------
# 5. System Installation Rules (Optional but highly recommended)
# ------------------------------------------------------------------------------
# These rules allow users to run `make install` or `cmake --install .` and
# properly copies the headers to their system (e.g. /usr/local/include).

include(GNUInstallDirs)

# Install the headers
install(DIRECTORY include/vrpc
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export the CMake targets so users can use `find_package(vrpc)`
install(TARGETS vrpc EXPORT vrpcTargets)
install(EXPORT vrpcTargets
    FILE vrpcTargets.cmake
    NAMESPACE vrpc::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vrpc
)
