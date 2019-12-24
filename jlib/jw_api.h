#pragma once
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#ifdef OS_WIN
#include <direct.h>
#include <io.h>
#define ACCESS _access
#define MKDIR(dir_name) _mkdir(dir_name)
#else
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#define ACCESS access
#define MKDIR(dir_name) mkdir(dir_name, 0755)
#endif

#ifdef OS_WIN
#define FD_SETSIZE (1024)
#include <WinSock.h>
#include <Windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef OS_WIN
typedef SOCKET socket_t;
typedef int32_t pid_t;
typedef int32_t socklen_t;
#else
typedef int socket_t;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#endif

#ifndef OS_WIN
#include <pthread.h>
#endif

#ifdef OS_WIN
#include <process.h>
#include <Shlwapi.h>
#else
#include <sys/syscall.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#endif

#ifdef OS_WIN
#define pthread_mutex_t CRITICAL_SECTION
#define pthread_mutex_init(x, y) InitializeCriticalSection(x)
#define pthread_mutex_destroy(x) DeleteCriticalSection(x)
#define pthread_mutex_lock(x) EnterCriticalSection(x)
#define pthread_mutex_unlock(x) LeaveCriticalSection(x)
#endif

namespace jw
{

//----------------------
// dir functions
//----------------------

//// create dir
bool dir_create(const char *dir);

//----------------------
// file functions
//----------------------

//// access file
bool file_access(const char *filename);

//----------------------
// process functions
//----------------------

//// get current process id
pid_t process_get_id(void);

//// get current process name
void process_get_module_name(char *module_name, size_t max_size);

//----------------------
// thread functions
//----------------------

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

//----------------------
// mutex functions
//----------------------
#ifdef OS_WIN
typedef LPCRITICAL_SECTION mutex_t;
#else
typedef pthread_mutex_t *mutex_t;
#endif

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
	auto_mutex(mutex_t m) : _m(m) { mutex_lock(_m); }
	~auto_mutex() { mutex_unlock(_m); }
	mutex_t _m;
};

//----------------------
// signal/semaphone functions
//----------------------

#ifdef OS_WIN
typedef HANDLE signal_t;
#else
typedef void *signal_t;
#endif

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
