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
				//�ڶ���д�������������� �ܵ����ѵ��쳣 SIGPIPE,
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
			//�ڷ�����ģʽ��,send�����Ĺ��̽����ǽ����ݿ�����Э��ջ�Ļ���������,������������ÿռ䲻��,�������Ŀ���,
			//�������سɹ������Ĵ�С;�绺�������ÿռ�Ϊ0,�򷵻�-1,ͬʱ����errnoΪEAGAIN.
			if (n > 0) {
				send_count += n;
				data->AdjustOffset(n);
				if (data->Length() == 0) {
					delete data;
					send_data_.erase(it);
					printf("����һ������ɹ�");
				}
				else {
					printf("����̫��,�����ܿ�����%d���ݵ����ͻ���", send_count);
					EnableWrite(true);
					break;
				}
			}
			else if (n <= 0) {
				int error = ErrerCode;
#ifdef _MSC_VER
				if (error == WSAEINTR) {
					printf("���ͱ��жϣ��´λص��ٷ�");
					break;
				}
				else if (error == WSAEWOULDBLOCK) {
					printf("��������");
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
		printf("ȫ���ͳɹ�");
	}
	
}

void TcpConn::HandleError() {
	fprintf(stderr, "TcpConn HandleError\n");
	Close();
}

//����memcpy�����б���Ҫ���͵����ݰ�
//���ⷢ��̫�������������ӷ��ͣ�ÿ���������ݰ����÷��ͻ�������С
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