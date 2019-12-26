#pragma once

#define JW_ENABLE_LOG 1

class noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}

private:
	noncopyable(const noncopyable &) = delete;
	const noncopyable &operator=(const noncopyable &) = delete;
};
