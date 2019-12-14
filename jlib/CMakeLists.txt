cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

#设置动态链接库的目录，以及头文件目录
include_directories(
    ./base
	./encrypt
	./log
	./net)
link_directories(
    ${PROJECT_SOURCE_DIR}/lib)


# 将源添加到此项目的可执行文件。
set(SOURCE_FILES
		"./base/jw_thread.cpp"
		"./log/jw_log.cpp"
		"./net/jw_socket.cpp"
		"./net/jw_buffer.cpp"
		"./net/jw_tcpclient.cpp"
		"./net/jw_tcpconn.cpp"
		"./net/jw_tcpserver.cpp"
        )
		
# add_definitions("-O0 -g3 -gdwarf-2 -fnessage-length=0 -MMD -MP")

MESSAGE(STATUS "################" ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "################" ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(jlib STATIC ${SOURCE_FILES})

