#pragma once
#include <sys/epoll.h>

namespace jw
{

//----------------------
// poller functions
//----------------------

//// 
void poller_create();

//// 
void poller_run();

//// 
void poller_destory();


} // namespace jw
