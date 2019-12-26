#pragma once

#include <list>
#include <mutex>


template <typename T>
struct LockQueue: public std::mutex {
	LockQueue() {}
	bool push(T &&v);
	bool pop(T *v);
	int32_t size();
	bool swap(std::list<T> &items);
	bool append(std::list<T>& items);
private:
	std::list<T> items_;
};

template <typename T>
int32_t LockQueue<T>::size() {
	std::lock_guard<std::mutex> lk(*this);
	return items_.size();
}

template <typename T>
bool LockQueue<T>::push(T &&v) {
	std::lock_guard<std::mutex> lk(*this);
	items_.push_back(std::move(v));
	return true;
}

template <typename T>
bool LockQueue<T>::pop(T *v) {
	std::unique_lock<std::mutex> lk(*this);
	*v = std::move(items_.front());
	items_.pop_front();
	return true;
}

template <typename T>
bool LockQueue<T>::swap(std::list<T> &items) {
	std::lock_guard<std::mutex> lk(*this);
	items.swap(items_);
	return true;
}




