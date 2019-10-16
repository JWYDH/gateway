#pragma once
#include "../stdafx.h"
#include <functional>

class BaseThread
{
private:
	
#ifdef _MSC_VER
	static void CALLBACK Hook(void* ptr);
#else
	static void* Hook(void* arg);
#endif
public:
	BaseThread();
	virtual ~BaseThread();
	int Start(std::function<void()> thread_func);
	//等待线程执行完毕，wait_time参数表示等待的最大毫秒数，INFINITE表示无限等待。
	//注意，调用此函数的线程在此线程执行完毕后会一直处于阻塞状态
	//参数wait_alert表示调用线程在阻塞期间是否允许进入警告状态（仅对于windows有效)
#ifdef _MSC_VER
	int waitFor(unsigned int wait_time = INFINITE, bool wait_alert = true);
#else
	int waitFor();
#endif
private:
#ifdef _MSC_VER
	void*			thread_;			//线程句柄
	unsigned long	thread_id_;			//线程ID
#else
	//int pid_; //进程id
	pthread_t		thread_id_;
#endif
	std::function<void()> thread_func_;
};

