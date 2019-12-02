#pragma once

#ifdef _MSC_VER
#define pthread_mutex_t					CRITICAL_SECTION
#define pthread_mutex_init(x,y)			InitializeCriticalSection(x)
#define pthread_mutex_destroy(x)		DeleteCriticalSection(x)
#define pthread_mutex_lock(x)			EnterCriticalSection(x)
#define pthread_mutex_unlock(x)			LeaveCriticalSection(x)
#endif

class ThreadMutex;
class ThreadGuard;
class Runnable;
class Thread;
//class RWLock;

class ThreadMutex{
public:
	ThreadMutex();
	virtual ~ThreadMutex();
	void Lock();
	//int Trylock();
	void Unlock();
protected:
	pthread_mutex_t mutex_;
};

class ThreadGuard{
public:
	ThreadGuard(ThreadMutex *mutex);
	virtual ~ThreadGuard();
private:
	ThreadMutex *mutex_;
};

