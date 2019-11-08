#include "jw_lock.h"


ThreadMutex::ThreadMutex(){
	pthread_mutex_init(&mutex_, NULL);
}

ThreadMutex::~ThreadMutex(){
	pthread_mutex_destroy(&mutex_);
}

void ThreadMutex::Lock(){
	pthread_mutex_lock(&mutex_);
}

//int ThreadMutex::Trylock(){
//	return pthread_mutex_trylock(&mutex_);
//}

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
