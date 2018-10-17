
file(READ ${CMAKE_SOURCE_DIR}/src/version.cc _VERSION_CC_CONTENTS)
string(REGEX MATCH "version_string = \"[0-9]+\\.[0-9]+" GPERF_VER "${_VERSION_CC_CONTENTS}")
string(REPLACE "version_string = \"" "" GPERF_VER "${GPERF_VER}")

if(BUILD_NUMBER)
    set(GPERF_VERSION ${GPERF_VER}.${BUILD_NUMBER})
else()
    set(GPERF_VERSION ${GPERF_VER}.0)
endif()
