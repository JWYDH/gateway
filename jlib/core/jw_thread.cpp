#include "jw_thread.h"

#include <unistd.h>
#include <pthread.h>

#include <sys/time.h>

#include "jw_atomic.h"

namespace jw
{

struct thread_data_s
{
	thread_id_t tid;
	thread_function entry_func;
	void *param;
	std::string name;
};
static __thread thread_data_s *thread_local_data = nullptr;

thread_id_t thread_get_current_id(void)
{
	return thread_local_data == nullptr ? ::pthread_self() : thread_local_data->tid;
}

const char *thread_get_current_name(void)
{
	return thread_local_data == nullptr ? "<UNNAME>" : thread_local_data->name.c_str();
}

thread_id_t thread_get_id(thread_t t)
{
	thread_data_s *data = (thread_data_s *)t;
	return data->tid;
}

void *_thread_entry(void *data)
{
	thread_local_data = (thread_data_s *)data;

	//set random seed
	::srand((uint32_t)::time(0));

	if (thread_local_data->entry_func)
		thread_local_data->entry_func(thread_local_data->param);
}

thread_t thread_create(thread_function func, void *param, const char *name)
{
	thread_data_s *data = new thread_data_s;
	data->entry_func = func;
	data->param = param;
	data->name = name ? name : "";
	::pthread_create(&data->tid, NULL, _thread_entry, data);
	
	return data;
}

void thread_sleep(int32_t msec)
{
	struct timeval time;
	time.tv_sec = msec / 1000;
	time.tv_usec = msec % 1000 * 1000;
	select(0, NULL, NULL, NULL, &time);
}

void thread_join(thread_t thread)
{
	thread_data_s *data = (thread_data_s *)thread;
	::pthread_join(data->tid, nullptr);
	delete data;
}

void thread_yield(void)
{
	::sched_yield();
}

mutex_t mutex_create(void)
{
	pthread_mutex_t *pm = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	::pthread_mutex_init(pm, 0);
	return pm;
}


void mutex_destroy(mutex_t m)
{
	::pthread_mutex_destroy(m);
	free(m);
}


void mutex_lock(mutex_t m)
{
	::pthread_mutex_lock(m);
}


void mutex_unlock(mutex_t m)
{
	::pthread_mutex_unlock(m);
}

struct signal_s
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	atomic_int32_t predicate;
};


signal_t signal_create(void)
{
	signal_s *sig = (signal_s *)malloc(sizeof(*sig));
	sig->predicate = 0;
	pthread_mutex_init(&(sig->mutex), 0);
	pthread_cond_init(&(sig->cond), 0);
	return (signal_t)sig;
}

void signal_destroy(signal_t s)
{
	signal_s *sig = (signal_s *)s;
	pthread_cond_destroy(&(sig->cond));
	pthread_mutex_destroy(&(sig->mutex));
	free(sig);
}

void signal_wait(signal_t s)
{
	signal_s *sig = (signal_s *)s;
	pthread_mutex_lock(&(sig->mutex));
	while (0 == sig->predicate.load())
	{
		pthread_cond_wait(&(sig->cond), &(sig->mutex));
	}
	sig->predicate = 0;
	pthread_mutex_unlock(&(sig->mutex));
}

bool _signal_unlock_wait(signal_s *sig, uint32_t ms)
{
	const uint64_t kNanoSecondsPerSecond = 1000ll * 1000ll * 1000ll;

	if (sig->predicate.load() == 1)
	{ //It's light!
		sig->predicate = 0;
		return true;
	}

	//need wait...
	if (ms == 0) {
		return false; //zero-timeout event state check optimization
	}

	timeval tv;
	gettimeofday(&tv, 0);
	uint64_t nanoseconds = ((uint64_t)tv.tv_sec) * kNanoSecondsPerSecond + ms * 1000 * 1000 + ((uint64_t)tv.tv_usec) * 1000;

	timespec ts;
	ts.tv_sec = (time_t)(nanoseconds / kNanoSecondsPerSecond);
	ts.tv_nsec = (long int)(nanoseconds - ((uint64_t)ts.tv_sec) * kNanoSecondsPerSecond);

	//wait...
	while (0 == sig->predicate.load())
	{
		if (pthread_cond_timedwait(&(sig->cond), &(sig->mutex), &ts) != 0){
			return false; //time out
		}
	}

	sig->predicate = 0;
	return true;
}

bool signal_timewait(signal_t s, uint32_t ms)
{
	signal_s *sig = (signal_s *)s;
	if (ms == 0)
	{
		if (EBUSY == pthread_mutex_trylock(&(sig->mutex)))
			return false;
	}
	else
	{
		pthread_mutex_lock(&(sig->mutex));
	}

	bool ret = _signal_unlock_wait(sig, ms);

	pthread_mutex_unlock(&(sig->mutex));
	return ret;
}

void signal_notify(signal_t s)
{
	signal_s *sig = (signal_s *)s;
	pthread_mutex_lock(&(sig->mutex));
	sig->predicate = 1;
	pthread_cond_signal(&(sig->cond));
	pthread_mutex_unlock(&(sig->mutex));
}

} // namespace jw
