#include "jw_tcpserver.h"

namespace jw
{

TcpServer::TcpServer()
{
}

TcpServer::~TcpServer()
{
}

void TcpServer::_server_func(void *)
{
	while (!stoped_)
	{
		const static int kMaxEvents = 32;
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
				break;
			}
		}
		for (int i = 0; i < n; i++)
		{
			int fd = evs[i].data.fd;
			int ev = evs[i].events;
			TcpConn *conn = (TcpConn *)(evs[i].data.ptr);
			if (fd == listen_fd_)
			{
				TcpConn conn;
				int connfd = jw::accept(listen_fd_, nullptr);
				if (connfd < 0)
				{
					JW_LOG(LL_ERROR, "unknown connfd connect");
					continue;
				}
				conn.socket_ = connfd;
				conn.conn_state_ = TcpServer::CONNSTATE_CONNECTED;
				jw::set_nonblock(conn.socket_, true);
				jw::set_recv_buf_size(conn.socket_, BUFF_SIZE::RECV_BUF_SIZE);
				jw::set_send_buf_size(conn.socket_, BUFF_SIZE::SEND_BUF_SIZE);
				jw::getsockname(conn.socket_, conn.local_addr_);
				jw::getpeername(conn.socket_, conn.remote_addr_);
				connects_.push_back(conn);
					
				ev.data.fd = connfd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

				cout << "accapt a connection from " << str << endl;
			}

			if (ev & (EPOLLERR | EPOLLHUP))
			{
				JW_LOG(LL_WARN, "epoll_wait triger %s %s",
					   ev | EPOLLERR ? "EPOLLERR" : "",
					   ev | EPOLLHUP ? "EPOLLHUP" : "",
					   ev | EPOLLIN ? "EPOLLIN" : "",
					   ev | EPOLLOUT ? "EPOLLOUT" : "");
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

	server_thread_ = jw::thread_create(this->_server_func, nullptr, "server thread");
	return true;
}

void TcpServer::Stop()
{
	stoped_ = true;
	jw::thread_join(server_thread_);
}
} // namespace jw