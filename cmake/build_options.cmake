
message(STATUS "Project ................ ${PROJECT_NAME}")
message(STATUS "Version ................ ${PROJECT_VERSION}")
message(STATUS "Generator .............. ${CMAKE_GENERATOR}")
message(STATUS "Build Type ............. ${CMAKE_BUILD_TYPE}")

option(BUILD_TESTS "Build test cases for gperf" OFF)

message(STATUS "BUILD_TESTS ............ ${BUILD_TESTS}")


file(TO_CMAKE_PATH "${CMAKE_INSTALL_PREFIX}" CMAKE_INSTALL_PREFIX) # fix up for windows install