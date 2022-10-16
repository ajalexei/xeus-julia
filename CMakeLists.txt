#############################################################################
# Copyright (c) 2022, Alexei Andreanov
#
# Distributed under the terms of the GNU General Public License v3.
#
# The full license is in the file LICENSE, distributed with this software.
#############################################################################

cmake_minimum_required(VERSION 3.4.3)

project(xeus-julia)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(XEUS_JULIA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)


# Versionning
# ===========

file(STRINGS "${XEUS_JULIA_INCLUDE_DIR}/xeus-julia/xeus_julia_config.hpp" version_defines
     REGEX "#define XEUS_JULIA_VERSION_(MAJOR|MINOR|PATCH)")

foreach (ver ${version_defines})
    if (ver MATCHES "#define XEUS_JULIA_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XEUS_JULIA_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif ()
endforeach ()

set(${PROJECT_NAME}_VERSION
    ${XEUS_JULIA_VERSION_MAJOR}.${XEUS_JULIA_VERSION_MINOR}.${XEUS_JULIA_VERSION_PATCH})
message(STATUS "Building xeus-julia v${${PROJECT_NAME}_VERSION}")


# Configuration
# =============

include(GNUInstallDirs)

if (NOT DEFINED XEUS_JULIA_KERNELSPEC_PATH)
    set(XEUS_JULIA_KERNELSPEC_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
endif ()

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xjulia/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xjulia/kernel.json"
)


# Build options
# =============

OPTION(XEUS_JULIA_DISABLE_ARCH_NATIVE "disable -march=native flag" OFF)
OPTION(XEUS_JULIA_DISABLE_TUNE_GENERIC "disable -mtune=generic flag" OFF)

option(XEUS_JULIA_BUILD_STATIC "Build xeus-julia static library" ON)
OPTION(XEUS_JULIA_BUILD_SHARED "Split xjulia build into executable and library" ON)
OPTION(XEUS_JULIA_BUILD_EXECUTABLE "Build the xjulia executable" ON)

OPTION(XEUS_JULIA_USE_SHARED_XEUS "Link xjulia with the xeus shared library (instead of the static library)" ON)
OPTION(XEUS_JULIA_USE_SHARED_XEUS_JULIA "Link xjulia with the xeus shared library (instead of the static library)" ON)


# Dependencies
# ============

# Fing pkg-config
find_package(PkgConfig REQUIRED)

# UUID
if(UNIX AND NOT APPLE)
    pkg_search_module(LibUUID uuid REQUIRED)
endif()

# if (TARGET PkgConfig::LibUUID)
#    message(STATUS "Found libuuid)
# endif ()

set(xtl_REQUIRED_VERSION 0.6.23)
set(xeus_REQUIRED_VERSION 2.0.0)
set(julia_REQUIRED_VERSION 1.7)

if (NOT TARGET xtl)
    find_package(xtl ${xtl_REQUIRED_VERSION} REQUIRED)
endif ()

if (NOT TARGET xeus AND NOT TARGET xeus-static)
    find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
endif ()

if (NOT TARGET julia)
#    find_package(julia ${julia_REQUIRED_VERSION} REQUIRED)
    pkg_search_module(JULIA julia REQUIRED)
endif ()


# Flags
# =====

include(CheckCXXCompilerFlag)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")

    if (XEUS_JULIA_DISABLE_ARCH_NATIVE AND NOT XEUS_JULIA_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
    elseif (XEUS_JULIA_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else ()
# Apple clang does not support -march=native
        if (NOT APPLE)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        endif ()
    endif ()

    CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP_17_FLAG)
    if (HAS_CPP_17_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    else ()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++17 support!")
    endif ()

endif ()


# Source files
# ============

set(XEUS_JULIA_HEADERS
    include/xeus-julia/xeus_julia_config.hpp
    include/xeus-julia/xinterpreter.hpp
)

set(XEUS_JULIA_SRC
    src/xinterpreter.cpp
)

set(XEUS_JULIA_MAIN_SRC
    src/main.cpp
)


# Targets and link - Macros
# =========================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

macro(xeus_julia_set_common_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4141)
        target_compile_options(${target_name} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        # Mtune generic/native
        if (XEUS_JULIA_DISABLE_ARCH_NATIVE AND NOT XEUS_JULIA_DISABLE_TUNE_GENERIC)
            target_compile_options(${target_name} PUBLIC -mtune=generic)
        elseif (XEUS_JULIA_DISABLE_TUNE_GENERIC)
        else ()
            target_compile_options(${target_name} PUBLIC -march=native)
        endif ()

        # C++17 flag
        CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP_17_FLAG)
        if (HAS_CPP_17_FLAG)
            target_compile_features(${target_name} PRIVATE cxx_std_17)
        else ()
            message(FATAL_ERROR "Unsupported compiler -- xeus-julia requires C++17 support!")
        endif ()
    endif ()

    if (APPLE)
        set_target_properties(${target_name} PROPERTIES
            MACOSX_RPATH ON
        )
    else ()
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH 1
            SKIP_BUILD_RPATH FALSE
        )
    endif ()

    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endmacro()

# Common macro kernels (xjulia )
macro(xeus_julia_set_kernel_options target_name)
    if (XEUS_JULIA_USE_SHARED_XEUS_JULIA)
        target_link_libraries(${target_name} PRIVATE xeus-julia)
        if(CMAKE_DL_LIBS)
            target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util)
        endif()
    else ()
        target_link_libraries(${target_name} PRIVATE xeus-julia-static)
    endif()

    find_package(Threads)
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})

endmacro()

# Common macro for shared and static library
macro(xeus_julia_create_target target_name linkage output_name)
    string(TOUPPER "${linkage}" linkage_upper)

    if (NOT ${linkage_upper} MATCHES "^(SHARED|STATIC)$")
        message(FATAL_ERROR "Invalid library linkage: ${linkage}")
    endif ()

    add_library(${target_name} ${linkage_upper} ${XEUS_JULIA_SRC} ${XEUS_JULIA_HEADERS})
    xeus_julia_set_common_options(${target_name})

    set_target_properties(${target_name} PROPERTIES
                          PUBLIC_HEADER "${XEUS_JULIA_HEADERS}"
                          PREFIX ""
                          VERSION ${${PROJECT_NAME}_VERSION}
                          SOVERSION ${XEUS_JULIA_VERSION_MAJOR}
                          OUTPUT_NAME "lib${output_name}")

    target_compile_definitions(${target_name} PUBLIC "XEUS_JULIA_EXPORTS")

    target_compile_features(${target_name} PRIVATE cxx_std_17)

    target_include_directories(${target_name}
                               PUBLIC
                               $<BUILD_INTERFACE:${XEUS_JULIA_INCLUDE_DIR}>
                               $<INSTALL_INTERFACE:include>)

    if (XEUS_JULIA_USE_SHARED_XEUS)
        set(XEUS_JULIA_XEUS_TARGET xeus)
    else ()
        set(XEUS_JULIA_XEUS_TARGET xeus-static)
    endif ()

    target_link_libraries(${target_name} PUBLIC ${XEUS_JULIA_XEUS_TARGET} xtl )
    if (WIN32 OR CYGWIN)
        #
    elseif (APPLE)
        target_link_libraries(${target_name} PRIVATE "-undefined dynamic_lookup")
    endif ()

    find_package(Threads) # TODO: add Threads as a dependence of xeus-static?
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})

endmacro()

# xeus-julia
# ==========

set(XEUS_JULIA_TARGETS "")

if (XEUS_JULIA_BUILD_SHARED)
    # Build libraries
    xeus_julia_create_target(xeus-julia SHARED xeus-julia)
    list(APPEND XEUS_JULIA_TARGETS xeus-julia)
endif ()

if (XEUS_JULIA_BUILD_STATIC)
    # On Windows, a static library should use a different output name
    # to avoid the conflict with the import library of a shared one.
    if (CMAKE_HOST_WIN32)
        xeus_julia_create_target(xeus-julia-static STATIC xeus-julia-static)
    else ()
        xeus_julia_create_target(xeus-julia-static STATIC xeus-julia)
    endif ()
    list(APPEND XEUS_JULIA_TARGETS xeus-julia-static)
endif ()


# xeus-julia
# ==========

if (XEUS_JULIA_BUILD_EXECUTABLE)
    add_executable(xjulia ${XEUS_JULIA_MAIN_SRC})
    target_compile_features(xjulia PRIVATE cxx_std_17)
    xeus_julia_set_common_options(xjulia)
    xeus_julia_set_kernel_options(xjulia)
endif()


# Installation
# ============

include(CMakePackageConfigHelpers)

set(XEUS_JULIA_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for xeus-juliaConfig.cmake")

# Install xeus-julia and xeus-julia-static
if (XEUS_JULIA_BUILD_SHARED)
    install(TARGETS ${XEUS_JULIA_TARGETS}
            EXPORT ${PROJECT_NAME}-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xeus-julia)

    # Makes the project importable from the build directory
    export(EXPORT ${PROJECT_NAME}-targets
           FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
endif ()

# Install xjulia
if (XEUS_JULIA_BUILD_EXECUTABLE)
    install(TARGETS xjulia
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Configuration and data directories for jupyter and xeus-julia
    set(XJUPYTER_DATA_DIR "share/jupyter"    CACHE STRING "Jupyter data directory")

    # Install xjulia Jupyter kernelspec
    set(KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
    install(DIRECTORY ${KERNELSPEC_DIR}
            DESTINATION ${XJUPYTER_DATA_DIR}
            PATTERN "*.in" EXCLUDE)


    # Extra path for installing Jupyter kernelspec
    if (XEXTRA_JUPYTER_DATA_DIR)
        install(DIRECTORY ${KERNELSPEC_DIR}
                DESTINATION ${XEXTRA_JUPYTER_DATA_DIR}
                PATTERN "*.in" EXCLUDE)
    endif ()
endif ()

# Configure 'xeus-juliaConfig.cmake' for a build tree
set(XEUS_JULIA_CONFIG_CODE "####### Expanded from \@XEUS_JULIA_CONFIG_CODE\@ #######\n")
set(XEUS_JULIA_CONFIG_CODE "${XEUS_JULIA_CONFIG_CODE}set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake;\${CMAKE_MODULE_PATH}\")\n")
set(XEUS_JULIA_CONFIG_CODE "${XEUS_JULIA_CONFIG_CODE}##################################################")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${PROJECT_BINARY_DIR})

# Configure 'xeus-juliaConfig.cmake.in for an install tree
set(XEUS_JULIA_CONFIG_CODE "")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XEUS_JULIA_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${XEUS_JULIA_CMAKECONFIG_INSTALL_DIR})

if (XEUS_JULIA_BUILD_SHARED)
    install(EXPORT ${PROJECT_NAME}-targets
            FILE ${PROJECT_NAME}Targets.cmake
            DESTINATION ${XEUS_JULIA_CMAKECONFIG_INSTALL_DIR})
endif ()
