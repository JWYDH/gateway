#include "jw_tcp_client.h"
namespace jw
{

TcpClient::TcpClient()
{
	stoped_ = false;

	memset(&local_addr_, 0, sizeof(local_addr_));
	memset(&remote_addr_, 0, sizeof(remote_addr_));

	conn_state_ = TcpClient::CONNSTATE_CLOSED;

	OnConnected([](TcpClient *conn) {

	});

	OnDisconnected([](TcpClient *conn) {

	});

	OnRead([](TcpClient *conn, RingBuf &buf) {

	});
}

TcpClient::~TcpClient()
{
	send_data_pending_.append(send_data_);
	for (auto &buf : send_data_)
	{
		delete buf;
	}
}

void TcpClient::Connected()
{
	conn_state_ = TcpClient::CONNSTATE_CONNECTED;

	jw::set_recv_buf_size(socket_, TcpClient::BUFF_SIZE::RECV_BUF_SIZE);
	jw::set_send_buf_size(socket_, TcpClient::BUFF_SIZE::SEND_BUF_SIZE);

	jw::getsockname(socket_, local_addr_);
	jw::getpeername(socket_, remote_addr_);

	JW_LOG(LL_INFO, "connected fd=%d, %s:%d ========= %s:%d",
		   socket_,
		   inet_ntoa(local_addr_.sin_addr), ntohs(local_addr_.sin_port),
		   inet_ntoa(remote_addr_.sin_addr), ntohs(remote_addr_.sin_port));
	connected_callback_(this);
}

void TcpClient::Disconnected()
{
	disconnected_callback_(this);
	conn_state_ = TcpClient::CONNSTATE_CLOSED;
	JW_LOG(LL_INFO, "disconnected fd=%d, %s:%d ========= %s:%d",
		   socket_, inet_ntoa(local_addr_.sin_addr), ntohs(local_addr_.sin_port),
		   inet_ntoa(remote_addr_.sin_addr), ntohs(remote_addr_.sin_port));

	jw::close_socket(socket_);
}

void TcpClient::_read_thread_func()
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socket_, &fds);
	int nRetAll = select(socket_ + 1, &fds, NULL, NULL, NULL);
	if (nRetAll > 0)
	{
		if (FD_ISSET(socket_, &fds))
		{
			if (recv_data_.size() > RECV_MAX_SIZE)
			{
				JW_LOG(LL_WARN, "too many data not proc!!!");
			}
			int32_t count = 0;
			for (;;)
			{
				int32_t n = 0;
				n = recv_data_.read_socket(socket_);

				if (n > 0)
				{
					count = +n;
					continue;
				}
				if (n <= 0)
				{
					int read_errno = errno;
					JW_LOG(LL_INFO, "tcpclient sum = %d, recv = %d", recv_data_.size(), count);
					if (recv_data_.size() > 0)
					{
						read_callback_(this, recv_data_);
					}
					if (!(read_errno == EAGAIN || read_errno == EWOULDBLOCK))
					{
						Disconnected();
					}
					break;
				}
			}
		}
	}
	else if (nRetAll == 0)
	{
		JW_LOG(LL_INFO, "tcpclient select time out: %d", errno);
	}
	else
	{
		if (errno != EINTR)
		{
			JW_LOG(LL_ERROR, "tcpclient select error: %d", errno);
		}
	}
}

void TcpClient::_write_thread_func()
{
	send_data_pending_.append(send_data_);
	if (send_data_.empty())
	{
		jw::thread_sleep(100);
		return;
	}
	for (auto it = send_data_.begin(); it != send_data_.end();)
	{
		auto &data = *it;
		int32_t n = 0;
		n = data->write_socket(socket_);

		if (n > 0)
		{
			JW_LOG(LL_INFO, "tcpclient send = %d", n);
			if (data->size() == 0)
			{
				delete data;
				it = send_data_.erase(it);
			}
			else
			{
				break;
			}
		}
		else if (n <= 0)
		{
			int write_errno = errno;
			if (write_errno == EAGAIN || write_errno == EWOULDBLOCK || write_errno == EINPROGRESS)
			{
				break;
			}
			else
			{
				JW_LOG(LL_INFO, "tcpclient send data fail, may be perr unnormal disconnect");
				break;
			}
		}
	}
}

bool TcpClient::Start(const char *ip, const short port)
{
	auto read_thread_func = [this, ip, port](void *) {
		while (!stoped_)
		{
			if (conn_state_ == TcpClient::CONNSTATE_CLOSED)
			{
				socket_ = jw::create_socket();
				struct sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = inet_addr(ip);
				addr.sin_port = htons(port);

				jw::set_nonblock(socket_, true);
				if (jw::connect(socket_, addr))
				{
					Connected();
				}
				else
				{
					int lasterr = jw::get_lasterror();
					//non-block socket
					if (lasterr == EINPROGRESS)
					{
						conn_state_ = TcpClient::CONNSTATE_CONNECTING;
					}
				}
			}
			if (conn_state_ == TcpClient::CONNSTATE_CONNECTING)
			{
				timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = 0;
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(socket_, &fds);
				int nRetAll = select(socket_ + 1, NULL, &fds, NULL, &tv);
				if (nRetAll > 0 && jw::get_socket_error(socket_) == 0)
				{
					Connected();
				}
				else
				{
					JW_LOG(LL_INFO, "tcpclient connect time out, auto reconnecting");
					jw::close_socket(socket_);
					conn_state_ = TcpClient::CONNSTATE_CLOSED;
					jw::thread_sleep(5000);
				}
			}
			if (conn_state_ == TcpClient::CONNSTATE_CONNECTED)
			{
				this->_read_thread_func();
			}
		}
		if (conn_state_ == TcpClient::CONNSTATE_CONNECTED)
		{
			jw::close_socket(socket_);
		}
	};

	auto write_thread_func = [this, ip, port](void *) {
		while (!stoped_)
		{
			if (conn_state_ != TcpClient::CONNSTATE_CONNECTED)
			{
				jw::thread_sleep(100);
			}
			this->_write_thread_func();
		}
	};

	read_thread_ = jw::thread_create(read_thread_func, nullptr, "tcpclient auto connect thread");
	write_thread_ = jw::thread_create(write_thread_func, nullptr, "tcpclient write thread");
	return true;
}

void TcpClient::Stop()
{
	stoped_ = true;
	jw::thread_join(read_thread_);
	jw::thread_join(write_thread_);
}

void TcpClient::DoWrite(const char *buf, int32_t len)
{
	if (stoped_)
	{
		return;
	}
	RingBuf *data = new RingBuf();
	data->write(buf, len);
	send_data_pending_.push(std::move(data));
}

} // namespace jw