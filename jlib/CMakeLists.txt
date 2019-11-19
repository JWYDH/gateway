# CMakeList.txt: jlib 的 CMake 项目，包括源和定义
# 此处特定于项目的逻辑。
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)

MESSAGE(STATUS "################" ${PROJECT_SOURCE_DIR})


MESSAGE(STATUS "################" ${PROJECT_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#add_definitions("-O0 -g3 -gdwarf-2 -fnessage-length=0 -MMD -MP")
#设置动态链接库的目录，以及头文件目录
set(INCLUDE_DIR include)
#set(LINK_DIR lib)

include_directories(${INCLUDE_DIR})
link_directories(${LINK_DIR})
#link_libraries(library_name)
# 将源添加到此项目的可执行文件。

set(SOURCE_FILES
        "jlib.cpp"
		"./log/jw_log.cpp"
		
        )

add_executable (jlib ${SOURCE_FILES})
#target_link_libraries(project_name library_name)
# 需要打入动态链接库的源文件
#set(SOURCE_FILES_FOR_SHARED
#       libm
#	   )
        
# add_library(library-name SHARED ${SOURCE_FILES_FOR_SHARED})

