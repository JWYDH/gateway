// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//
#pragma once



//头文件

#ifdef _MSC_VER
#include <WinSock2.h>  
#include <Windows.h>
typedef int socklen_t;
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include "errno.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
typedef int SOCKET;
//#pragma region define win32 const variable in linux
#define INVALID_SOCKET	-1
#define SOCKET_ERROR	-1
//#pragma endregion
#include <sys/epoll.h>

#include <pthread.h>
//#include <semaphore.h>
#include <sys/syscall.h>

#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <locale.h>
//
#ifdef _MSC_VER
#pragma comment(lib, "wsock32")
#endif

//错误码
#ifdef _MSC_VER
#define ErrerCode WSAGetLastError()
#else
#define ErrerCode errno
#endif // _MSC_VER

// TODO: 在此处引用程序需要的其他头文件
