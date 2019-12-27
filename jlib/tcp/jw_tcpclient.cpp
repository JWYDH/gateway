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

	OnRead([this](TcpClient *conn, char *buf, int32_t len) -> int32_t {
		//DoWrite(buf, len);
		return len;
	});
}

TcpClient::~TcpClient()
{
	for (auto &buf : send_data_)
	{
		delete buf;
	}
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
			

		} // end if break;
	}
	else if (nRetAll == 0)
	{
		JW_LOG(LL_INFO, "select time out: %d\n", errno);
	}
	else
	{
		JW_LOG(LL_ERROR, "GetRecvBufSize error: %d\n", errno);
	}
}

void TcpClient::_write_thread_func()
{
	write_msg_.append(send_data_);
	if (send_data_.empty())
	{
		return true;
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
	return false;
}

bool TcpClient::Start(const char *ip, const short port)
{
	auto thread_func = [this, ip, port]() {
		while (!stoped_)
		{
			if (conn_state_ == TcpClient::CONNSTATE_CONNECTED)
			{
				this->Loop();
			}
			else
			{
				socket_ = jw::create_socket();

				if (socket_.Connect(ip, port))
				{
					Connected();
				}
				else
				{
					socket_.Close();
					printf("connnect fial\n");
					jw::thread_sleep(1000);
				}
			}
		}
		socket_.Close();
	};
	thread_.Start(thread_func);

	auto write_thread_func = [this, ip, port]() {
		while (!stoped_)
		{
			if (conn_state_ != TcpClient::CONNSTATE_CONNECTED)
			{
				Sleep(1);
			}
			if (this->HandleWrite())
			{
				Sleep(1);
			}
		}
		socket_.Close();
	};
	write_thread_.Start(write_thread_func);
	return true;
}

void TcpClient::Stop()
{
	stoped_ = true;
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

	jw::close_socket(socket_)
}

void TcpClient::HandleRead()
{
	if (recv_data_.Length() > RECV_MAX_SIZE)
	{
		printf("too many data not proc, so close\n");
		jw::close_socket(socket_);
		return;
	}
	int32_t count = 0;
	for (;;)
	{
		int32_t n = 0;
		if (recv_data_.AvaliableCapacity() < RECV_ONCV_SIZE)
		{
			recv_data_.ResetSize(recv_data_.Capacity() * 2);
		}
		n = socket_.Recv(recv_data_.DataEndPtr(), recv_data_.AvaliableCapacity(), 0);
		if (n > 0)
		{
			count += n;
			recv_data_.AdjustDataEnd(n);
			continue;
		}
		if (n == 0)
		{
			printf("close normal\n");
			MyMethod();
			Disconnected();
			break;
		}
		if (n < 0)
		{

			int error = errno;
			if (error == EINTR)
			{
				continue;
			}
			else if (error == EAGAIN || error == EWOULDBLOCK || error == EINPROGRESS)
			{
				break;
			}
			else
			{
				printf("read expect %d\n", error);
				MyMethod();
				Disconnected();
				break;
			}
		}
	}
}

void TcpClient::DoWrite(const char *buf, int32_t len)
{
	if (stoped_)
	{
		printf("PushInterMsgxx fail\n");
		return;
	}
	Buffer *data = new Buffer(RECV_ONCV_SIZE);
	data->WriteBuff(buf, len);
	inter_msg_.push(std::move(data));
}

} // namespace jw