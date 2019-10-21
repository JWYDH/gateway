#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <limits>

#include <list>
#include <mutex>
#include <thread>
#include <vector>


template <typename T>
struct SafeQueue: public std::mutex {
	SafeQueue() {}
	bool push(T &&v);
	bool pop(T *v);
	int32_t size();
	bool swap(std::list<T> &items);
	bool append(std::vector<T>& items);
private:
	std::list<T> items_;
};

template <typename T>
int32_t SafeQueue<T>::size() {
	std::lock_guard<std::mutex> lk(*this);
	return items_.size();
}

template <typename T>
bool SafeQueue<T>::push(T &&v) {
	std::lock_guard<std::mutex> lk(*this);
	items_.push_back(std::move(v));
	return true;
}

template <typename T>
bool SafeQueue<T>::pop(T *v) {
	std::unique_lock<std::mutex> lk(*this);
	*v = std::move(items_.front());
	items_.pop_front();
	return true;
}

template <typename T>
bool SafeQueue<T>::swap(std::list<T> &items) {
	std::lock_guard<std::mutex> lk(*this);
	items.swap(items_);
	return true;
}

template <typename T>
bool SafeQueue<T>::append(std::vector<T>& items) {
	std::lock_guard<std::mutex> lk(*this);

	for (auto it = items_.begin(); it != items_.end();)
	{
		items.push_back(std::move(*it));
		it = items_.erase(it);
	}
	return true;
}



