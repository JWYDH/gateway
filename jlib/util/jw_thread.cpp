#include "jw_thread.h"

BaseThread::BaseThread()
{
#ifdef _MSC_VER
	thread_ = NULL;
#endif
	thread_id_ = 0;
}

BaseThread::~BaseThread()
{
	if (thread_id_!=0)
	{
#ifdef _MSC_VER
		if (thread_)
		{
			CloseHandle(thread_);
			thread_ = NULL;
		}
		thread_id_ = 0;
#endif
	}
}

#ifdef _MSC_VER
void CALLBACK BaseThread::Hook(void* thread_ptr)
{
	BaseThread* thread = (BaseThread*)thread_ptr;
	thread->thread_func_();
	thread->terminated_ = true;
	ExitThread(0);
}
#else
void* BaseThread::Hook(void* thread_ptr)
{
	BaseThread* thread = (BaseThread*)thread_ptr;
	thread->thread_func_();
	thread->terminated_ = true;
	return (void*)NULL;
}
#endif

int BaseThread::Start(std::function<void()> thread_func) {
	thread_func_ = thread_func;
	assert(thread_func_);
#ifdef _MSC_VER
	thread_ = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BaseThread::Hook, this, 0, &thread_id_);
	return 0;
#else
	int ret = pthread_create(&thread_id_, NULL, BaseThread::Hook, this);
	return ret;
#endif
}

#ifdef _MSC_VER
int BaseThread::waitFor(unsigned int wait_time, bool wait_alert)
{
	return WaitForSingleObjectEx(thread_, wait_time, wait_alert);
}
#else
int BaseThread::waitFor()
{
	return pthread_join(thread_id_, NULL);
}
#endif




