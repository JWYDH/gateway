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
			int events = evs[i].events;
			if (fd == listen_fd_)
			{
				TcpConn *conn = new TcpConn;
				int connfd = jw::accept(listen_fd_, nullptr);
				if (connfd < 0)
				{
					JW_LOG(LL_ERROR, "unknown connfd connect");
					continue;
				}
				conn->socket_ = connfd;
				conn->conn_state_ = TcpConn::CONNSTATE_CONNECTED;
				jw::set_nonblock(conn->socket_, true);
				jw::set_recv_buf_size(conn->socket_, TcpConn::BUFF_SIZE::RECV_BUF_SIZE);
				jw::set_send_buf_size(conn->socket_, TcpConn::BUFF_SIZE::SEND_BUF_SIZE);
				jw::getsockname(conn->socket_, conn->local_addr_);
				jw::getpeername(conn->socket_, conn->remote_addr_);

				auto it = connects_.find(connfd);
				if (it != connects_.end())
				{
					connects_.erase(it);
				}
				connects_[connfd] = conn;

				struct epoll_event ev;
				ev.data.fd = connfd;
				ev.events = EPOLLIN | EPOLLET;
				ev.data.ptr = conn;
				epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, connfd, &ev);

				JW_LOG(LL_INFO, "accapt a connection fd %d from %s:%d\n",
					   connfd,
					   inet_ntoa(conn->local_addr_.sin_addr), ntohs(conn->local_addr_.sin_port),
					   inet_ntoa(conn->remote_addr_.sin_addr), ntohs(conn->remote_addr_.sin_port));

				if (events & (EPOLLERR | EPOLLHUP))
				{
					JW_LOG(LL_WARN, "epoll_wait triger %s %s %s %s",
						   events | EPOLLERR ? "EPOLLERR" : "",
						   events | EPOLLHUP ? "EPOLLHUP" : "",
						   events | EPOLLIN ? "EPOLLIN" : "",
						   events | EPOLLOUT ? "EPOLLOUT" : "");
					continue;
				}
			}
			else
			{
				TcpConn *conn = (TcpConn *)(evs[i].data.ptr);
				if (events & EPOLLIN)
				{
					if (conn->recv_data_.size() > TcpConn::BUFF_SIZE::RECV_MAX_SIZE)
					{
						JW_LOG(LL_WARN, "tcpconn too many data not proc!!!\n");
					}
					int32_t count = 0;
					for (;;)
					{
						int32_t n = 0;
						n = conn->recv_data_.read_socket(conn->socket_);
						if (n > 0)
						{
							count = +n;
							continue;
						}
						if (n <= 0)
						{
							JW_LOG(LL_INFO, "tcpconn sum = %d, recv = %d\n", conn->recv_data_.size(), count);
							if (conn->recv_data_.size() > 0)
							{
								read_callback_(conn, conn->recv_data_);
							}
							if (n < 0)
							{
								if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS))
								{
									CloseTcpConn(conn);
								}
								break;
							}
						}
					}
					read_callback_(conn, conn->recv_data_);
				}
				if (events & EPOLLOUT)
				{

					conn->send_data_pending_.append(conn->send_data_);
					for (auto it = conn->send_data_.begin(); it != conn->send_data_.end();)
					{
						auto &data = *it;
						int32_t n = 0;
						n = data->write_socket(conn->socket_);
						if (n > 0)
						{
							JW_LOG(LL_INFO, "tcpconn send = %d\n", n);
							if (data->size() == 0)
							{
								delete data;
								it = conn->send_data_.erase(it);
							}
							else
							{
								break;
							}
						}
						else if (n <= 0)
						{
							if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
							{
								break;
							}
							else
							{
								JW_LOG(LL_INFO, "tcpconn send data fail, may be perr unnormal disconnect\n");
								break;
							}
						}
					}
				}
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
		JW_LOG(LL_ERROR, "tcpserver bind error: %d\n", errno);
		return false;
	}

	if (!jw::listen(listen_fd_))
	{
		JW_LOG(LL_ERROR, "tcpserver listen error: %d\n", errno);
		return false;
	}

	server_thread_ = jw::thread_create(this->_server_func, nullptr, "tcpserver thread");
	return true;
}

void TcpServer::Stop()
{
	stoped_ = true;
	jw::thread_join(server_thread_);
}

void TcpServer::CloseTcpConn(TcpConn *conn)
{
	auto it = connects_.find(conn->socket_);
	if (it != connects_.end())
	{
		connects_.erase(it);
	}
	disconnected_callback_(conn);
	jw::close_socket(conn->socket_);
}
} // namespace jw