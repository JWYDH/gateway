cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

#设置头文件目录
include_directories(
	../jlib)
#    ../jlib/base
#	../jlib/encrypt
#	../jlib/log
#	../jlib/net)

#设置动态链接库目录
link_directories(
    ../jlib/lib)


# 库文件集合
set(LIBRARY_FILES
        #pthread
        jlib
        )

# 源文件代码集合
set(SOURCE_FILES
#		"test_log.hpp"
#		"test_mysql.hpp"
#		"test_tcpclient.hpp"
#		"test_tcpserver.hpp"
        "main.cpp")

# add_definitions("-O0 -g3 -gdwarf-2 -fnessage-length=0 -MMD -MP")

MESSAGE(STATUS "################" ${PROJECT_SOURCE_DIR})
# MESSAGE(STATUS "################" ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_executable(test ${SOURCE_FILES})
add_dependencies(test jlib)
target_link_libraries(test ${LIBRARY_FILES})
