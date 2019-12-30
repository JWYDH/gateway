#include "jw_tcpclient.h"
namespace jw
{

TcpClient::TcpClient()
{
	stoped_ = false;

	memset(&local_addr_, 0, sizeof(local_addr_));
	memset(&remote_addr_, 0, sizeof(remote_addr_));

	conn_state_ = TcpClient::CONNSTATE_CLOSED;

	OnConnected([this](TcpClient *conn) {

	});

	OnDisconnected([this](TcpClient *conn) {

	});

	OnRead([this](TcpClient *conn) {

	});
}

TcpClient::~TcpClient()
{
	for (auto &buf : send_data_)
	{
		delete buf;
	}
}

void TcpClient::Connected()
{
	conn_state_ = TcpClient::CONNSTATE_CONNECTED;
	jw::set_nonblock(socket_, true);
	jw::set_recv_buf_size(socket_, BUFF_SIZE::RECV_BUF_SIZE);
	jw::set_send_buf_size(socket_, BUFF_SIZE::SEND_BUF_SIZE);
	JW_LOG(LL_INFO, "connected fd=%d, %s:%d ========= %s:%d\n",
		   socket_,
		   inet_ntoa(local_addr_.sin_addr), ntohs(local_addr_.sin_port),
		   inet_ntoa(remote_addr_.sin_addr), ntohs(remote_addr_.sin_port));
	connected_callback_(this);
}

void TcpClient::Disconnected()
{

	disconnected_callback_(this);
	conn_state_ = TcpClient::CONNSTATE_CLOSED;
	JW_LOG(LL_INFO, "disconnected fd=%d, %s:%d ========= %s:%d\n",
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
				JW_LOG(LL_WARN, "too many data not proc!!!\n");
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
				if (n == 0)
				{
					read_callback_(this);
					Disconnected();
					break;
				}
				if (n < 0)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
					{
						JW_LOG(LL_INFO, "sum = %d, recv = %d\n", recv_data_.size(), count);
						read_callback_(this);
						break;
					}
					else
					{
						JW_LOG(LL_INFO, "is not normal close: %d\n", errno);
						Disconnected();
						break;
					}
				}
			}
		}
	}
	else if (nRetAll == 0)
	{
		JW_LOG(LL_INFO, "select time out: %d\n", errno);
	}
	else
	{
		if (errno != EINTR)
		{
			JW_LOG(LL_ERROR, "select error: %d\n", errno);
		}
	}
}

void TcpClient::_write_thread_func()
{
	send_data_pending.append(send_data_);
	if (send_data_.empty())
	{
		jw::thread_sleep(100);
		return;
	}
	int32_t send_count = 0;
	for (auto it = send_data_.begin(); it != send_data_.end();)
	{
		auto &data = *it;
		int32_t n = 0;
		n = socket_.Send(data->OffsetPtr(), data->AvaliableLength());
		if (n > 0)
		{
			send_count += n;
			data->AdjustOffset(n);
			if (data->AvaliableLength() == 0)
			{
				delete data;
				it = send_data_.erase(it);
				printf("·¢ËÍÒ»¸ö´ó°ü³É¹¦\n");
			}
			else
			{
				printf("Êý¾ÝÌ«´ó,¾¡¿ÉÄÜ¿½±´ÁË%dÊý¾Ýµ½·¢ËÍ»º³å\n", send_count);
				break;
			}
		}
		else if (n <= 0)
		{
			int error = errno;
			if (error == EINTR)
			{
				break;
			}
			else if (error == EAGAIN || error == EWOULDBLOCK)
			{
				break;
			}
			else
			{
				printf("write expect %d\n", error);
				break;
			}
		}
	}
	if (send_data_.empty())
	{
		printf("È«·¢ËÍ³É¹¦\n");
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

				if (jw::connect(socket_, addr))
				{
					Connected();
				}
				else
				{
					JW_LOG(LL_INFO, "connect time out, auto reconnect after 1 seconds\n");
					jw::close_socket(socket_);
					jw::thread_sleep(1000);
				}
			}
			else if (conn_state_ == TcpClient::CONNSTATE_CONNECTING)
			{
			}
			else if (conn_state_ == TcpClient::CONNSTATE_CONNECTED)
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

	jw::thread_create(read_thread_func, nullptr, "auto connect thread");
	jw::thread_create(write_thread_func, nullptr, "forever write thread");
	return true;
}

void TcpClient::Stop()
{
	stoped_ = true;
}

void TcpClient::DoWrite(const char *buf, int32_t len)
{
	if (stoped_)
	{
		return;
	}
	Buffer *data = new Buffer();
	data->WriteBuff(buf, len);
	send_data_pending.push(std::move(data));
}

} // namespace jw