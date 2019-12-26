#pragma once

#include <stdint.h>

#include <functional>

namespace jw
{

//----------------------
// thread functions
//----------------------

typedef void *thread_t;
typedef pthread_t thread_id_t;
//// thread entry function
typedef std::function<void(void *)> thread_function;

//// get current thread id
thread_id_t thread_get_current_id(void);

//// get current thread name
const char *thread_get_current_name(void);

//// get the system id of thread
thread_id_t thread_get_id(thread_t t);

//// create a new thread(use thread_join to release resources)
thread_t thread_create(thread_function func, void *param, const char *name, bool detached=false);

//// sleep in current thread(milliseconds)
void thread_sleep(int32_t msec);

//// wait the thread to terminate
void thread_join(thread_t t);

//// yield the processor
void thread_yield(void);

//----------------------
// mutex functions
//----------------------
typedef pthread_mutex_t *mutex_t;

/// create a mutex
mutex_t mutex_create(void);

/// destroy a mutex
void mutex_destroy(mutex_t m);

/// lock mutex(wait other owner unlock)
void mutex_lock(mutex_t m);

/// unlock mutex
void mutex_unlock(mutex_t m);

/// auto lock
struct auto_mutex
{
	auto_mutex(mutex_t m) : m_(m) { mutex_lock(m_); }
	~auto_mutex() { mutex_unlock(m_); }
	mutex_t m_;
};

//----------------------
// signal/semaphone functions
//----------------------

typedef void *signal_t;

//// create a signal
signal_t signal_create(void);

//// destroy a signal
void signal_destroy(signal_t s);

//// wait a signal inifinite
void signal_wait(signal_t s);

//// wait a signal in [t] millisecond(second*1000), return true immediately if the signal is lighted, if false if timeout or other error
bool signal_timewait(signal_t s, uint32_t ms);

//// light the signal
void signal_notify(signal_t s);

} // namespace jw