#include "jw_tcp_server.h"

namespace jw
{

TcpServer::TcpServer()
{
	OnConnected([](TcpConn *conn) {

	});

	OnDisconnected([](TcpConn *conn) {

	});

	OnRead([](TcpConn *conn, RingBuf &buf) {

	});
}

TcpServer::~TcpServer()
{
}

void TcpServer::_set_event(TcpConn *conn, int operation, uint32_t events)
{
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.fd = conn->socket_;
	event.events = events;
	event.data.ptr = (void *)conn;
	if (::epoll_ctl(epoll_fd_, operation, conn->socket_, &event) < 0)
	{
		JW_LOG(LL_ERROR, "tcpserver epoll_ctl error, err=%d %s", errno, strerror(errno));
	}
}

void TcpServer::_server_func(void *)
{
	epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event event;
	memset(&event, 0, sizeof(event));
	event.data.fd = listen_fd_;
	event.events = EPOLLIN | EPOLLET;
	::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event);

	while (!stoped_)
	{
		const static int kMaxEvents = 32;
		struct epoll_event evs[kMaxEvents];
		int n = epoll_wait(epoll_fd_, evs, kMaxEvents, 1000);
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
				conn->conn_state_ = TcpConn::CONNSTATE_CONNECTED;
				conn->server_ = this;
				conn->socket_ = connfd;
				_set_event(conn, EPOLL_CTL_ADD, EPOLLIN | EPOLLET);

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

				JW_LOG(LL_INFO, "%s:%d accapt a connection from %s:%d fd=%d",
					   inet_ntoa(conn->local_addr_.sin_addr), ntohs(conn->local_addr_.sin_port),
					   inet_ntoa(conn->remote_addr_.sin_addr), ntohs(conn->remote_addr_.sin_port),
					   connfd);

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
						JW_LOG(LL_WARN, "tcpconn too many data not proc!!!");
					}
					int32_t count = 0;
					for (;;)
					{
						int32_t rn = 0;
						rn = conn->recv_data_.read_socket(conn->socket_);
						
						if (rn > 0)
						{
							count = +rn;
							continue;
						}
						if (rn <= 0)
						{
							int read_errno = errno;
							JW_LOG(LL_INFO, "tcpconn sum = %d, recv = %d", conn->recv_data_.size(), count);
							if (conn->recv_data_.size() > 0)
							{
								read_callback_(conn, conn->recv_data_);
							}

							if (!(read_errno == EAGAIN || read_errno == EWOULDBLOCK))
							{
								Close(conn);
							}
							break;
						}
					}
				}
				if (events & EPOLLOUT)
				{
					_set_event(conn, EPOLL_CTL_MOD, EPOLLIN | EPOLLET);
					conn->send_data_pending_.append(conn->send_data_);
					for (auto it = conn->send_data_.begin(); it != conn->send_data_.end();)
					{
						auto &data = *it;
						int32_t wn = 0;
						wn = data->write_socket(conn->socket_);
						
						if (wn > 0)
						{
							JW_LOG(LL_INFO, "tcpconn send succ = %d", n);
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
						else if (wn <= 0)
						{
							int write_errno = errno;
							if (write_errno == EAGAIN || write_errno == EWOULDBLOCK)
							{
								_set_event(conn, EPOLL_CTL_MOD, EPOLLIN | EPOLLET | EPOLLOUT);
								break;
							}
							JW_LOG(LL_INFO, "tcpconn send data fail, may be peer unnormal disconnect");
							break;
						}
					}
				}
			}
		}
	}
}
bool TcpServer::Start(const char *ip, const short port)
{

	listen_fd_ = jw::create_socket();

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	if (!jw::bind(listen_fd_, addr))
	{
		JW_LOG(LL_ERROR, "tcpserver bind error: %d", errno);
		return false;
	}

	jw::set_recv_buf_size(listen_fd_, TcpConn::BUFF_SIZE::RECV_BUF_SIZE);
	jw::set_send_buf_size(listen_fd_, TcpConn::BUFF_SIZE::SEND_BUF_SIZE);

	if (!jw::listen(listen_fd_))
	{
		JW_LOG(LL_ERROR, "tcpserver listen error: %d", errno);
		return false;
	}

	server_thread_ = jw::thread_create(std::bind(&TcpServer::_server_func, this, std::placeholders::_1), nullptr, "tcpserver thread");
	return true;
}

void TcpServer::Stop()
{
	stoped_ = true;
	jw::thread_join(server_thread_);
}

void TcpServer::DoWrite(TcpConn *conn, const char *buf, int32_t len)
{
	RingBuf *data = new RingBuf();
	data->write(buf, len);
	conn->send_data_pending_.push(std::move(data));
	_set_event(conn, EPOLL_CTL_MOD, EPOLLIN | EPOLLET | EPOLLOUT);
}
void TcpServer::Close(TcpConn *conn)
{
	_set_event(conn, EPOLL_CTL_DEL, 0);
	auto it = connects_.find(conn->socket_);
	if (it == connects_.end())
	{
		assert(true);
	}
	connects_.erase(it);
	conn->conn_state_ = TcpConn::CONNSTATE_CLOSED;
	JW_LOG(LL_INFO, "disconnected fd=%d, %s:%d ========= %s:%d",
		   conn->socket_, inet_ntoa(conn->local_addr_.sin_addr), ntohs(conn->local_addr_.sin_port),
		   inet_ntoa(conn->remote_addr_.sin_addr), ntohs(conn->remote_addr_.sin_port));
	disconnected_callback_(conn);

	jw::close_socket(conn->socket_);
	delete conn;
}

} // namespace jw