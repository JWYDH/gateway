#include "jw_tcpserver.h"

TcpServer::TcpServer()
{
}

TcpServer::~TcpServer(){
}

void TcpServer::_server_func() {
	while (true) {
		const static int kMaxEvents = 20;
		struct epoll_event evs[kMaxEvents];
		int n = epoll_wait(listen_fd_, evs, kMaxEvents, 0);
		if (n == -1) {
			if (ErrerCode == EINTR) {
				continue;
			}
			else {
				printf( "epoll_wait error %d %s", ErrerCode, strerror(ErrerCode));
				break;
			}
		}
		for (int i = 0; i < n; i++) {
			void* ptr = evs[i].data.ptr;
			TcpConn* io = (TcpConn*)ptr;
			int ev = evs[i].events;
			if (ev & (EPOLLERR | EPOLLHUP)) {
				printf( "epoll_wait triger %s %s ", ev | EPOLLERR ? "EPOLLERR" : "", ev | EPOLLERR ? "EPOLLHUP" : "");
				io->HandleError();
				continue;
			}
			if (ev & EPOLLIN) {
				io->HandleRead();
			}
			if (ev & EPOLLOUT) {
				io->HandleWrite();
			}
		}
	}

}

bool TcpServer::Start(const char *ip, const short port, int back_log/*=256*/){
	epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);

	jw::create_socket(listen_fd_);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	if(!jw::bind(listen_fd_, addr))
	{
		JW_LOG(LL_ERROR,"bind error: %d\n", errno);
		return false;
	}
	
	if (!jw::listen(listen_fd_);) {
		socket_.Close();
		JW_LOG(LL_ERROR,"listen error: %d\n", errno);
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
