#pragma once

#include <list>
#include <mutex>

#include "jw_thread.h"

namespace {

template <typename T>
class LockQueue {
	LockQueue() {}
	bool push(T &&v);
	bool pop(T *v);
	int32_t size();
	bool swap(std::list<T> &items);
private:
	std::list<T> items_;
	mutex_t *lock_;
};

template <typename T>
int32_t LockQueue<T>::size() {
	jw::auto_mutex(lock_);
	return items_.size();
}

template <typename T>
bool LockQueue<T>::push(T &&v) {
	jw::auto_mutex(lock_);
	items_.push_back(std::move(v));
	return true;
}

template <typename T>
bool LockQueue<T>::pop(T *v) {
	jw::auto_mutex(lock_);
	*v = std::move(items_.front());
	items_.pop_front();
	return true;
}

template <typename T>
bool LockQueue<T>::swap(std::list<T> &items) {
	jw::auto_mutex(lock_);
	items.swap(items_);
	return true;
}

}


