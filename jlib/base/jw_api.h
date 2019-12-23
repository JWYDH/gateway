#pragma once
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#ifdef OS_WINDOWS
#define FD_SETSIZE (1024)
#include <WinSock.h>
#include <Windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef OS_WINDOWS
typedef SOCKET	socket_t;
typedef int32_t pid_t;
typedef int32_t socklen_t;
#else
typedef int		socket_t;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET	(-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR	(-1)
#endif	
#endif

#ifndef OS_WINDOWS
#include <pthread.h>
#endif

#ifdef OS_WINDOWS
#include <process.h>
#include <Shlwapi.h>
#else
#include <sys/syscall.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#endif

namespace jw
{
	//获取进程id
	pid_t get_process_id(void);

	//获取当前执行程序文件名
	void get_filename(char* module_name, size_t max_size);

	//获取当前线程id
	int64_t get_current_thread_id(void)
	{
		std::thread::id thread_id = std::this_thread::get_id();
		return thread_id;
	}

	//根据线程指针获取该线程id
	thread_id_t thread_get_id(thread_t t)
	{
		thread_data_s* data = (thread_data_s*)t;
		return data->tid;
	}
}
