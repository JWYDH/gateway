cmake_minimum_required (VERSION 2.8.1)

project ("test")

#设置头文件目录
include_directories(
	../jlib)

#设置动态链接库目录
link_directories(
	../jlib/lib)


# 库文件集合
set(LIBRARY_FILES
	jlib
	)

# 源文件代码集合
set(SOURCE_FILES
#	"test_log.hpp"
#	"test_mysql.hpp"
	"test_tcp_client.hpp"
	"test_tcp_server.hpp"
	"main.cpp")

SET(CMAKE_CXX_COMPILER "g++")
add_definitions("-std=c++11 -O0 -g3 -gdwarf-2 -fmessage-length=0 -MMD -MP")

MESSAGE(STATUS "################" ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "################" ${PROJECT_BINARY_DIR})
MESSAGE(STATUS "################" ${CMAKE_SOURCE_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/out)

add_executable(test ${SOURCE_FILES})
add_dependencies(test jlib)
target_link_libraries(test ${LIBRARY_FILES})
