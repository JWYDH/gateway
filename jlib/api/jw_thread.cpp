#include "jw_thread.h"

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
	return thread_data == 0 ? std::this_thread::get_id() : thread_data->tid;
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

} // namespace jw
