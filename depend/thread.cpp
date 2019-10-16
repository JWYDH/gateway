#include "jw_thread.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>


ThreadMutex::ThreadMutex(){
	const int r = pthread_mutex_init(&mutex_, nullptr);
	assert(r == 0);
}

ThreadMutex::~ThreadMutex(){
	pthread_mutex_destroy(&mutex_);
}

void ThreadMutex::Lock(){
	pthread_mutex_lock(&mutex_);
}

int ThreadMutex::Trylock(){
	return pthread_mutex_trylock(&mutex_);
}

void ThreadMutex::Unlock(){
	pthread_mutex_unlock(&mutex_);
}

ThreadGuard::ThreadGuard(ThreadMutex *mutex){
	mutex_ = nullptr;
	if (mutex){
		mutex_ = mutex;
		mutex_->Lock();
	}
}

ThreadGuard::~ThreadGuard(){
	if (mutex_){
		mutex_->Unlock();
	}
}


ThreadCond::ThreadCond(){
	pthread_cond_init(&cond_, nullptr);
}

ThreadCond::~ThreadCond(){
	pthread_cond_destroy(&cond_);
}

bool ThreadCond::Wait(int milliseconds){
	bool ret = true;
	if (milliseconds == 0){
		pthread_cond_wait(&cond_, &mutex_);
	}
	else{
		struct timeval cur_time;
		struct timespec abs_time;
		gettimeofday(&cur_time, NULL);
		int64_t us = (static_cast<int64_t>(cur_time.tv_sec) *
			static_cast<int64_t>(1000000) +
			static_cast<int64_t>(cur_time.tv_usec) +
			static_cast<int64_t>(milliseconds) *
			static_cast<int64_t>(1000));

		
			abs_time.tv_sec = (us / static_cast<int64_t>(1000000));
			abs_time.tv_nsec = (us % static_cast<int64_t>(1000000)) * 1000;
			ret = (pthread_cond_timedwait(&cond_, &mutex_, &abs_time) == 0);
	}
	return ret;
}

void ThreadCond::Signal(){
	pthread_cond_signal(&cond_);
}

void ThreadCond::Broadcast(){
	pthread_cond_broadcast(&cond_);
}

Sem::Sem(){
};

Sem::~Sem(){
	if (strlen(name_) == 0){
		sem_destroy(&sem_id_);
	}
	else{
		sem_close(ps_id_);
		sem_unlink(name_);
		memset(name_, 0, sizeof(name_));
	}
}

bool Sem::Create(int init_count, const char *name){
	if (init_count < 0){
		init_count = 0;
	}
	if (name == nullptr || strlen(name) == 0){
		if (sem_init(&sem_id_, 0, init_count) != 0){
			return false;
		}
	}
	else{
		if (strlen(name) >= 256){
			return false;
		}
		strcpy(name_, name);
		ps_id_ = sem_open(name, O_CREAT, 0644, init_count);
		if (ps_id_ == SEM_FAILED){
			return false;
		}
	}
	return true;
}

bool Sem::Open(const char *name){
	if (name == nullptr || strlen(name) == 0 || strlen(name) >= 256){
		return false;
	}
	strcpy(name_, name);
	ps_id_ = sem_open(name, O_EXCL);
	if (ps_id_ == SEM_FAILED){
		return false;
	}
	return true;
}

bool Sem::Wait(int time_out){
	if (strlen(name_) == 0){
		if (time_out <= 0){
			return (sem_wait(&sem_id_) == 0);
		}
		else{
			timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += time_out / 1000;
			ts.tv_nsec += (time_out % 1000) * 1000000;
			return (sem_timedwait(&sem_id_, &ts) == 0);
		}
	}
	else{
		if (time_out <= 0){
			return (sem_wait(ps_id_) == 0);
		}
		else{
			timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += time_out / 1000;
			ts.tv_nsec += (time_out % 1000) * 1000000;
			return (sem_timedwait(ps_id_, &ts) == 0);
		}
	}
}

bool Sem::Post(){
	if (strlen(name_) == 0){
		return (sem_post(&sem_id_) == 0);
	}
	else {
		return (sem_post(ps_id_) == 0);
	}
}

Thread::Thread(){ 
	tid_ = 0;
	pid_ = 0;
	runnable_ = nullptr;
	args_ = nullptr;
}

Thread::~Thread(){
};

Runnable* Thread::GetRunnable() {
	return runnable_;
}

void* Thread::GetArgs() {
	return args_;
}

bool Thread::Start(Runnable *runnable, void *args){
	assert(runnable);
	runnable_ = runnable;
	args_ = args;
	return pthread_create(&tid_, nullptr, Thread::Hook, this);
}

void Thread::Join() {
	if (tid_) {
		pthread_join(tid_, nullptr);
		tid_ = 0;
		pid_ = 0;
	}
}

void *Thread::Hook(void *arg) {
	Thread *thread = (Thread*)arg;
	thread->GetRunnable()->Run(thread, thread->GetArgs());
	return (void*)nullptr;
}

RWLock::RWLock(LOCK_MODE lock_mode) {
	pthread_rwlockattr_t attr;
	pthread_rwlockattr_init(&attr);
	if (lock_mode == LOCK_MODE_WRITE_PRIORITY) {
		pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	}
	else if (lock_mode == LOCK_MODE_READ_PRIORITY) {
		pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_READER_NP);
	}
	pthread_rwlock_init(&rwlock_, &attr);
}

RWLock::~RWLock() {
	pthread_rwlock_destroy(&rwlock_);
}

int RWLock::RDLock() {
	return pthread_rwlock_rdlock(&rwlock_);
}

int RWLock::WRLock() {
	return pthread_rwlock_wrlock(&rwlock_);
}

int RWLock::TryRDLock() {
	return pthread_rwlock_tryrdlock(&rwlock_);
}

int RWLock::TryWRLock() {
	return pthread_rwlock_trywrlock(&rwlock_);
}

int RWLock::Unlock() {
	return pthread_rwlock_unlock(&rwlock_);
}