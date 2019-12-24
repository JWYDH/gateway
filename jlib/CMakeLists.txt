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
	./encrypt
	./container
	./net)
link_directories(
	${PROJECT_SOURCE_DIR}/lib)


# 将源添加到此项目的可执行文件。
set(SOURCE_FILES
		"jw_api.cpp"
#		"jw_log.cpp"
#		"jw_socket_api.cpp"
#		"./log/jw_log.cpp"
#		"./net/jw_socket.cpp"
#		"./net/jw_buffer.cpp"
#		"./net/jw_tcpclient.cpp"
#		"./net/jw_tcpconn.cpp"
#		"./net/jw_tcpserver.cpp"
		)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 11)
add_definitions("-O0 -g3 -gdwarf-2 -fmessage-length=0 -MMD -MP")

MESSAGE(STATUS "################" ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "################" ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out)

add_library(jlib STATIC ${SOURCE_FILES})

