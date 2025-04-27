cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# specify the cross compiler
SET(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(TARGET_COMPILER_NAME gcc)

project(cpp_restful_boost_beast VERSION 1.0.0)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" ${CMAKE_MODULE_PATH})

include(Project)

init_project()

find_package(Boost 1.74.0 REQUIRED)
find_package(GTest REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra")

if(ENABLE_TSAN)
	set(TSAN_STATUS "ON")
	set(SANITIZER_FLAGS "-fsanitize=thread -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
	set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
else()
	set(TSAN_STATUS "OFF")
endif()

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(PROJECT_OPEN_SOURCE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(test)

message(STATUS "====================================")
message(STATUS "C++ standard:                 " ${CMAKE_CXX_STANDARD})
message(STATUS "Unit Testing:                 " ${UNIT_TESTING})
message(STATUS "ThreadSanitizer:              " ${TSAN_STATUS})
message(STATUS "====================================")
