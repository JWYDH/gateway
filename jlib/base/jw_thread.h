#pragma once
#ifdef WIN32
#include <Windows.h>
#else
#include "errno.h"
#include <pthread.h>
#endif
#include <assert.h>
#include <functional>

class BaseThread
{
private:
	
#ifdef WIN32
	static void CALLBACK Hook(void* ptr);
#else
	static void* Hook(void* arg);
#endif
public:
	BaseThread();
	virtual ~BaseThread();
	int Start(std::function<void()> thread_func);
#ifdef WIN32
	//�ȴ��߳�ִ����ϣ�wait_time������ʾ�ȴ�������������INFINITE��ʾ���޵ȴ���
	//ע�⣬���ô˺������߳��ڴ��߳�ִ����Ϻ��һֱ��������״̬
	//����wait_alert��ʾ�����߳��������ڼ��Ƿ��������뾯��״̬��������windows��Ч)
	int waitFor(unsigned int wait_time = INFINITE, bool wait_alert = true);
#else
	int waitFor();
#endif
private:
#ifdef _MSC_VER
	void*			thread_;			//�߳̾��
	unsigned long	thread_id_;			//�߳�ID
#else
	//int pid_; //����id
	pthread_t		thread_id_;
#endif
	std::function<void()> thread_func_;
	bool terminated_;
};

