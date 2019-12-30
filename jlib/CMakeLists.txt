cmake_minimum_required (VERSION 2.8.1)

project ("jlib")
# The version number.
set(mylib_version_major 1)
set(mylib_version_minor 0)

configure_file (
	"${PROJECT_SOURCE_DIR}/config.h.in"
	"${PROJECT_SOURCE_DIR}/config.h"
	)

#设置动态链接库的目录，以及头文件目录
include_directories(
	.
	./api)
link_directories(
	${PROJECT_SOURCE_DIR}/lib)


# 将源添加到此项目的可执行文件。
set(SOURCE_FILES
	"./core/jw_file.cpp"
	"./core/jw_time.cpp"
	"./core/jw_process.cpp"
	"./core/jw_thread.cpp"
	"./core/jw_log.cpp"
	"./core/jw_socket.cpp"
	"./core/jw_ring_buffer.cpp"
	"./net/jw_tcp_client.cpp"
	)

set(CMAKE_CXX_COMPILER "g++")
add_definitions("-std=c++11 -O0 -g3 -gdwarf-2 -fmessage-length=0 -MMD -MP")

MESSAGE(STATUS "################" ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "################" ${PROJECT_BINARY_DIR})
MESSAGE(STATUS "################" ${CMAKE_SOURCE_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out)

add_library(jlib STATIC ${SOURCE_FILES})

