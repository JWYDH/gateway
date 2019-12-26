#include "jw_thread.h"

#include <pthread.h>

namespace jw
{

struct thread_data_s
{
	thread_id_t tid;
	std::thread thandle;
	thread_function entry_func;
	void *param;
	std::string name;
	bool detached;
	signal_t resume_signal;
};

static thread_local thread_data_s *thread_data = nullptr;

thread_id_t thread_get_current_id(void)
{
	return thread_data == 0 ? pthread_self() : thread_data->tid;
}

thread_id_t thread_get_id(thread_t t)
{
	thread_data_s *data = (thread_data_s *)t;
	return data->tid;
}

void _thread_entry(thread_data_s *data)
{
	thread_data = data;
	signal_wait(data->resume_signal);
	signal_destroy(data->resume_signal);
	data->resume_signal = 0;

	//set random seed
	srand((uint32_t)::time(0));

	if (data->entry_func)
		data->entry_func(data->param);

	thread_data = nullptr;
	if (data->detached)
	{
		delete data;
	}
}

thread_t _thread_create(thread_function func, void *param, const char *name, bool detached)
{
	thread_data_s *data = new thread_data_s;
	data->param = param;
	data->entry_func = func;
	data->detached = detached;
	data->name = name ? name : "";
	data->resume_signal = signal_create();
	data->thandle = std::thread(_thread_entry, data);
	data->tid = data->thandle.get_id();

	//detached thread
	if (data->detached)
		data->thandle.detach();

	//resume thread
	signal_notify(data->resume_signal);
	return data;
}

thread_t thread_create(thread_function func, void *param, const char *name)
{
	return _thread_create(func, param, name, false);
}

void thread_create_detached(thread_function func, void *param, const char *name)
{
	_thread_create(func, param, name, true);
}

void thread_sleep(int32_t msec)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

void thread_join(thread_t thread)
{
	thread_data_s *data = (thread_data_s *)thread;
	data->thandle.join();

	if (!(data->detached))
	{
		delete data;
	}
}

const char *thread_get_current_name(void)
{
	return (thread_data == nullptr) ? "<UNNAME>" : thread_data->name.c_str();
}

void thread_yield(void)
{
	std::this_thread::yield();
}

mutex_t mutex_create(void)
{
#ifdef OS_WIN
	LPCRITICAL_SECTION cs = (LPCRITICAL_SECTION)malloc(sizeof(CRITICAL_SECTION));
	::InitializeCriticalSection(cs);
	return cs;
#else
	pthread_mutex_t* pm = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	::pthread_mutex_init(pm, 0);
	return pm;
#endif
}

//-------------------------------------------------------------------------------------
void mutex_destroy(mutex_t m)
{
#ifdef OS_WIN
	::DeleteCriticalSection(m);
#else
	::pthread_mutex_destroy(m);
	free(m);
#endif
}

//-------------------------------------------------------------------------------------
void mutex_lock(mutex_t m)
{
#ifdef OS_WIN
	::EnterCriticalSection(m);
#else
	::pthread_mutex_lock(m);
#endif
}

//-------------------------------------------------------------------------------------
void mutex_unlock(mutex_t m)
{
#ifdef OS_WIN
	::LeaveCriticalSection(m);
#else
	::pthread_mutex_unlock(m);
#endif
}


#ifndef OS_WIN
struct signal_s
{
	pthread_mutex_t mutex;
	pthread_cond_t	cond;
	atomic_int32_t  predicate;
};
#endif

//-------------------------------------------------------------------------------------
signal_t signal_create(void)
{
#ifdef OS_WIN
	return ::CreateEvent(0, FALSE, FALSE, 0);
#else
	signal_s *sig = (signal_s*)malloc(sizeof(*sig));
	sig->predicate = 0;
	pthread_mutex_init(&(sig->mutex), 0);
	pthread_cond_init(&(sig->cond), 0);
	return (signal_t)sig;
#endif
}

void signal_destroy(signal_t s)
{
#ifdef OS_WIN
	::CloseHandle(s);
#else
	signal_s* sig = (signal_s*)s;
	pthread_cond_destroy(&(sig->cond));
	pthread_mutex_destroy(&(sig->mutex));
	free(sig);
#endif
}

void signal_wait(signal_t s)
{
#ifdef OS_WIN
	::WaitForSingleObject(s, INFINITE);
#else
	signal_s* sig = (signal_s*)s;
	pthread_mutex_lock(&(sig->mutex));
	while (0==sig->predicate.load()) {
		pthread_cond_wait(&(sig->cond), &(sig->mutex));
	}
	sig->predicate = 0;
	pthread_mutex_unlock(&(sig->mutex));
#endif
}

#ifndef OS_WIN
bool _signal_unlock_wait(signal_s* sig, uint32_t ms)
{
	const uint64_t kNanoSecondsPerSecond = 1000ll * 1000ll * 1000ll;

	if (sig->predicate.load() == 1) { //It's light!
		sig->predicate = 0;
		return true;
	}

	//need wait...
	if (ms == 0) return  false;	//zero-timeout event state check optimization

	timeval tv;
	gettimeofday(&tv, 0);
	uint64_t nanoseconds = ((uint64_t)tv.tv_sec) * kNanoSecondsPerSecond + ms * 1000 * 1000 + ((uint64_t)tv.tv_usec) * 1000;

	timespec ts;
	ts.tv_sec = (time_t)(nanoseconds / kNanoSecondsPerSecond);
	ts.tv_nsec = (long int)(nanoseconds - ((uint64_t)ts.tv_sec) * kNanoSecondsPerSecond);
	
	//wait...
	while(0 == sig->predicate.load()) {
		if (pthread_cond_timedwait(&(sig->cond), &(sig->mutex), &ts) != 0)
			return false; //time out
	}

	sig->predicate = 0;
	return true;
}
#endif

bool signal_timewait(signal_t s, uint32_t ms)
{
#ifdef OS_WIN
	return (WAIT_OBJECT_0 == ::WaitForSingleObject(s, ms));
#else
	signal_s* sig = (signal_s*)s;
	if (ms == 0) {
		if (EBUSY == pthread_mutex_trylock(&(sig->mutex)))
			return false;
	}
	else {
		pthread_mutex_lock(&(sig->mutex));
	}

	bool ret = _signal_unlock_wait(sig, ms);

	pthread_mutex_unlock(&(sig->mutex));
	return ret;
#endif
}

void signal_notify(signal_t s)
{
#ifdef OS_WIN
	::SetEvent(s);
#else
	signal_s* sig = (signal_s*)s;
	pthread_mutex_lock(&(sig->mutex));
	sig->predicate = 1;
	pthread_cond_signal(&(sig->cond));
	pthread_mutex_unlock(&(sig->mutex));
#endif
}

} // namespace jw
