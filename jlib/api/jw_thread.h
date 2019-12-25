#pragma once

#include <functional>
#include <thread>

namespace jw
{

//----------------------
// thread functions
//----------------------

typedef void *thread_t;
typedef std::thread::id thread_id_t;
//// thread entry function
typedef std::function<void(void *)> thread_function;

//// get current thread id
thread_id_t thread_get_current_id(void);

//// get the system id of thread
thread_id_t thread_get_id(thread_t t);

//// create a new thread(use thread_join to release resources)
thread_t thread_create(thread_function func, void *param, const char *name);

//// create a new thread(all thread resources will be released automatic)
void thread_create_detached(thread_function func, void *param, const char *name);

//// sleep in current thread(milliseconds)
void thread_sleep(int32_t msec);

//// wait the thread to terminate
void thread_join(thread_t t);

//// get current thread name
const char *thread_get_current_name(void);

//// yield the processor
void thread_yield(void);

} // namespace jw
