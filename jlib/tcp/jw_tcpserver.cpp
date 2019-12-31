#include "jw_tcpserver.h"

namespace jw
{

TcpServer::TcpServer()
{
}

TcpServer::~TcpServer()
{
}

void TcpServer::_server_func()
{
	const static int kMaxEvents = 20;
	struct epoll_event evs[kMaxEvents];
	int n = epoll_wait(listen_fd_, evs, kMaxEvents, 0);
	if (n == -1)
	{
		if (errno == EINTR)
		{
			return;
		}
		else
		{
			JW_LOG(LL_ERROR, "epoll_wait error %d %s", errno, strerror(errno));
			return;
		}
	}
	for (int i = 0; i < n; i++)
	{
		void *ptr = evs[i].data.ptr;
		TcpConn *io = (TcpConn *)ptr;
		int ev = evs[i].events;
		if (ev & (EPOLLERR | EPOLLHUP))
		{
			JW_LOG(LL_WARN, "epoll_wait triger %s %s ", ev | EPOLLERR ? "EPOLLERR" : "", ev | EPOLLERR ? "EPOLLHUP" : "");
			continue;
		}
		if (ev & EPOLLIN)
		{
			
		}
		if (ev & EPOLLOUT)
		{
			
		}
	}
}

bool TcpServer::Start(const char *ip, const short port)
{
	epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);

	listen_fd_ = jw::create_socket();

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	if (!jw::bind(listen_fd_, addr))
	{
		JW_LOG(LL_ERROR, "bind error: %d\n", errno);
		return false;
	}

	if (!jw::listen(listen_fd_))
	{
		JW_LOG(LL_ERROR, "listen error: %d\n", errno);
		return false;
	}

	auto server_thread_func = [this](void *) {
		while (!stoped_)
		{
			this->_server_func();
		}
	};
	server_thread_ = jw::thread_create(server_thread_func, nullptr, "server thread");
	return true;
}

void TcpServer::Stop()
{
	stoped_ = true;
	jw::thread_join(server_thread_);
}
} // namespace jw