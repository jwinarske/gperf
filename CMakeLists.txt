
cmake_minimum_required(VERSION 3.11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to MinSizeRel.")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")

include (get_version)


set(PACKAGE_NAME gperf)
project(${PACKAGE_NAME} VERSION "${GPERF_VERSION}" LANGUAGES CXX C)

include(build_options)
include(config_header)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    if(${TARGET_ARCH} STREQUAL "ARM" OR ${TARGET_ARCH} STREQUAL "ARM64")
        add_definitions(-D_ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE=1)
    endif()
endif()

SET(CMAKE_DEBUG_POSTFIX d)


# Library
file(GLOB lib_src_files lib/*.cc)
list(APPEND lib_src_files lib/getopt.c lib/getopt1.c)
add_library(gp-lib STATIC ${lib_src_files})
target_include_directories(gp-lib PUBLIC lib)
set_target_properties(gp-lib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    OUTPUT_NAME gp
)

# Executable
file(GLOB gperf_src_files src/*.cc)
add_executable(gperf ${gperf_src_files})
target_include_directories(gperf PUBLIC lib src)
target_link_libraries(gperf gp-lib)
add_dependencies(gperf gp-lib)
set_target_properties(gperf PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
if(GPERF_LIBM)
    target_link_libraries(gperf -lm)
endif()


include(GNUInstallDirs)
install (TARGETS gp-lib gperf
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install (FILES 
    ${CMAKE_SOURCE_DIR}/lib/getline.h
    ${CMAKE_SOURCE_DIR}/lib/getopt.h
    ${CMAKE_SOURCE_DIR}/lib/hash.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_custom_target(
    uninstall "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/cmake/make_uninstall.cmake"
)


include(FindLATEX)
if(LATEX_FOUND)
    add_subdirectory(doc)
endif()

if(BUILD_TESTS)
    set(GPERF_BIN ${CMAKE_BINARY_DIR}/gperf)
    MESSAGE(STATUS "GPERF_BIN=${GPERF_BIN}")
    add_subdirectory(tests)
endif()
