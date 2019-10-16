#include "jw_tcpconn.h"
#include "jw_tcpserver.h"

TcpConn::TcpConn(TcpServer* events): events_(events){
	conn_state_ = TcpConn::CONNSTATE_CLOSED;

	OnConnected( [this](TcpConn* conn) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		fprintf(stdout, "OnConnected fd=%d, %s ========= %s\n", (int)socket, inet_ntoa(local_addr.sin_addr), inet_ntoa(local_addr.sin_addr));
	});

	OnDisconnected([this](TcpConn* conn) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		fprintf(stdout, "OnDisconnected fd=%d, %s ====|==== %s\n", (int)socket, inet_ntoa(local_addr.sin_addr), inet_ntoa(local_addr.sin_addr));
	});

	OnRead([this](TcpConn* conn, char* buf, int32_t len) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		fprintf(stdout, "OnDisconnected fd=%d, %s ====|==== %s\n", (int)socket, inet_ntoa(local_addr.sin_addr), inet_ntoa(local_addr.sin_addr));
	});
}

TcpConn::~TcpConn() {
}

void TcpConn::EnableWrite(bool enable) {
	events_->SetWriteEvent((SOCKET)socket_, this, true);
}

void TcpConn::Connected() {
	socket_.SetNoBlock();
	if (!socket_.SetRecvBufSize(RECV_BUF_SIZE)) {
		fprintf(stderr, "set recv %d %s\n", ErrerCode, strerror(ErrerCode));
		return;
	}
	if (!socket_.SetSendBufSize(SEND_BUF_SIZE)) {
		fprintf(stderr, "set send %d %s\n", ErrerCode, strerror(ErrerCode));
		return;
	}
	connected_callback_(this);
}



void TcpConn::Disconnected() {
	disconnected_callback_(this);
}

void TcpConn::HandleRead() {
	int32_t count = 0;
	for (;;) {
		int32_t n = 0;
		Buffer* data = new Buffer();
		n = socket_.Recv(data->OffsetPtr(), data->Length(), 0);
		if (n > 0) {
			count += n;
			data->SetLength(n);
			recv_data_.push_back(data);
		}
		else if (n == 0) {
			if (count > 0)
			{
				Buffer bufs(count);
				printf("create buf size = %d\n", count);
				for (auto &buf : recv_data_)
				{
					bufs.WriteBuff(buf->OffsetPtr(), buf->Length());
					delete buf;
				}
				recv_data_.clear();
				bufs.Seek(0);
				printf("recv data count n= %d\n", bufs.Length());
				read_callback_(this, bufs.OffsetPtr(), bufs.Length());
			}
			Close();
			break;
		}
		else if (n < 0) {
			int error = ErrerCode;
#ifdef _MSC_VER
			if (error == WSAEINTR) {
				continue;
			}
			else if (error == WSAEWOULDBLOCK) {
				if (count > 0)
				{
					Buffer bufs(count);
					printf("create buf size = %d\n", count);
					for (auto &buf : recv_data_)
					{
						bufs.WriteBuff(buf->OffsetPtr(), buf->Length());
						delete buf;
					}
					recv_data_.clear();
					bufs.Seek(0);
					printf("recv data count n= %d\n", bufs.Length());
					read_callback_(this, bufs.OffsetPtr(), bufs.Length());
				}
				break;
			}
#else
			if (error == EINTR) {
				continue;
			}
			else if (error == EAGAIN || error == EWOULDBLOCK || error == EINPROGRESS) {
				break;
			}
#endif
			else {
				//第二次写？？？断了连接 管道破裂的异常 SIGPIPE,
				//if (count > 0)
				//{
				//	Buffer bufs(count);
				//	printf("create buf size = %d\n", count);
				//	for (auto &buf : recv_data_)
				//	{
				//		bufs.WriteBuff(buf->OffsetPtr(), buf->Length());
				//		delete buf;
				//	}
				//	recv_data_.clear();
				//	bufs.Seek(0);
				//	printf("recv data count n= %d\n", bufs.Length());
				//	read_callback_(this, bufs.OffsetPtr(), bufs.Length());
				//}
				Close();
				fprintf(stderr, "read expect %d\n", error);
				break;
			}
		}
	}
}

void TcpConn::HandleWrite() {
	EnableWrite(false);
	if (send_data_.empty()) {
		return;
	}
	int32_t send_count = 0;
	for (auto it = send_data_.begin(); it != send_data_.end(); ) {
		auto &data = *it;
		if (data->Length() > 0) {
			int32_t n = 0;
			n = socket_.Send(data->OffsetPtr(), data->Length(), 0);
			//在非阻塞模式下,send函数的过程仅仅是将数据拷贝到协议栈的缓存区而已,如果缓存区可用空间不够,则尽能力的拷贝,
			//立即返回成功拷贝的大小;如缓存区可用空间为0,则返回-1,同时设置errno为EAGAIN.
			if (n > 0) {
				send_count += n;
				data->AdjustOffset(n);
				if (data->Length() == 0) {
					delete data;
					send_data_.erase(it);
					printf("发送一个大包成功");
				}
				else {
					printf("数据太大,尽可能拷贝了%d数据到发送缓冲", send_count);
					EnableWrite(true);
					break;
				}
			}
			else if (n <= 0) {
				int error = ErrerCode;
#ifdef _MSC_VER
				if (error == WSAEINTR) {
					printf("发送被中断，下次回调再发");
					break;
				}
				else if (error == WSAEWOULDBLOCK) {
					printf("缓冲区满");
					break;
				}
#else
				if (error == EINTR) {
					break;
				}
				else if (error == EAGAIN || error == EWOULDBLOCK) {
					break;
				}
#endif
				else {
					fprintf(stderr, "write expect %d\n", error);
				}
			}
		}
	}
	if (send_data_.empty()) {
		printf("全发送成功");
	}
	
}

void TcpConn::HandleError() {
	fprintf(stderr, "TcpConn HandleError\n");
	Close();
}

//减少memcpy，用列表保存要发送的数据包
//避免发送太快阻塞其他连接发送，每个发送数据包设置发送缓冲区大小
void TcpConn::DoWrite(const char *buf, int32_t len) {
	if (send_data_.empty()) {
		EnableWrite(true);
	}
	auto it = send_data_.rbegin();
	if (it != send_data_.rend())
	{
		auto data = (*it);
		if (data->AvaliableCapacity() >= len)
		{
			data->WriteBuff(buf, len);
			return;
		}
	}
	Buffer *data = new Buffer(SEND_BUF_SIZE);
	data->WriteBuff(buf, len);
	send_data_.push_back(data);
}

void TcpConn::Close() {
	conn_state_ = TcpConn::CONNSTATE_CLOSED;
	socket_.Close();
	Disconnected();
}