
include(CheckFunctionExists)

#
# Check for stack-allocated variable-size arrays")
#
set(DYNAMIC_ARRAY_DETECT_CODE "int func (int n) { int dynamic_array[n]; }")

file(WRITE "${CMAKE_BINARY_DIR}/dynamic_array_detect.cc" "${DYNAMIC_ARRAY_DETECT_CODE}")

try_run(run_result_unused compile_result_unused
    "${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/dynamic_array_detect.cc"
    COMPILE_OUTPUT_VARIABLE HAVE_DYNAMIC_ARRAY)

string(REGEX MATCH "error" ERROR_DYNAMIC_ARRAY "${HAVE_DYNAMIC_ARRAY}")

if(NOT ERROR_DYNAMIC_ARRAY)
    set(HAVE_DYNAMIC_ARRAY TRUE)
    message(STATUS "Compiler has dynamic array support")
else()
    set(HAVE_DYNAMIC_ARRAY FALSE)
endif()

configure_file(cmake/config.h.in ${CMAKE_BINARY_DIR}/config.h @ONLY)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_BINARY_DIR})

#
# check if linking with -lm is required
#
set(CMAKE_REQUIRED_LIBRARIES m)
check_function_exists(rand GPERF_LIBM)
if(GPERF_LIBM)
    message(STATUS "Gperf requireds linking to -lm")
endif()

#
# target architecture
#
include (target_arch)
get_target_arch(TARGET_ARCH)
message(STATUS "Target ................. ${TARGET_ARCH}")
