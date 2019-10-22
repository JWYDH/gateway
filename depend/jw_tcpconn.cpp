#include "jw_tcpconn.h"
#include "jw_tcpserver.h"

TcpConn::TcpConn(TcpServer* events): events_(events){
	conn_state_ = TcpConn::CONNSTATE_CLOSED;

	OnConnected( [this](TcpConn* conn) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		
		printf( "OnConnected fd=%d, %s:%d ========= %s:%d\n", (int)socket_
			, inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port), inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
	});

	OnDisconnected([this](TcpConn* conn) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		printf("OnDisconnected fd=%d, %s:%d ========= %s:%d\n", (int)socket_
			, inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port), inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
	});

	OnRead([this](TcpConn* conn, char* buf, int32_t len)->int32_t {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		printf("OnRead fd=%d, %s:%d ========= %s:%d\n", (int)socket_
			, inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port), inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
		return true;
	});
}

TcpConn::~TcpConn() {
	for (auto& buf : free_data_)
	{
		delete buf;
	}
	for (auto& buf : send_data_)
	{
		delete buf;
	}
}

void TcpConn::EnableWrite(bool enable) {
	events_->SetWriteEvent((SOCKET)socket_, this, enable);
}

void TcpConn::Connected() {
	socket_.SetNoBlock();
	if (!socket_.SetRecvBufSize(RECV_BUF_SIZE)) {
		printf( "set recv %d\n", ErrerCode);
		return;
	}
	if (!socket_.SetSendBufSize(SEND_BUF_SIZE)) {
		printf( "set send %d\n", ErrerCode);
		return;
	}
	conn_state_ = TcpConn::CONNSTATE_CONNECTED;
	connected_callback_(this);
}



void TcpConn::Disconnected() {
	disconnected_callback_(this);
}

void TcpConn::HandleRead() {
	if (recv_data_.Length() > RECV_MAX_SIZE)
	{
		printf("too many data not proc, so close\n");
		Close();
		return;
	}
	int32_t count = 0;
	for (;;) {
		int32_t n = 0;
		if (recv_data_.AvaliableCapacity() < RECV_ONCV_SIZE){
			recv_data_.ResetSize(recv_data_.Capacity() * 2);
		}
		n = socket_.Recv(recv_data_.DataEndPtr(), recv_data_.AvaliableCapacity(), 0);
		if (n > 0) {
			count += n;
			recv_data_.AdjustDataEnd(n);
			continue;
		}
		if (n == 0) {
			//����Ӧ�ô�����δ���������ٹ�����
			printf("close normal\n");
			Close();
			break;
		}
		if (n < 0) {
			int error = ErrerCode;
#ifdef _MSC_VER
			if (error == WSAEINTR) {
				continue;
			}
			else if (error == WSAEWOULDBLOCK) {
				if (count > 0)
				{
					printf("this time recv data sum = %d\n", count);
					printf("this time need proc sum = %d\n", recv_data_.AvaliableLength());
					int32_t read_count = read_callback_(this, recv_data_.OffsetPtr(), recv_data_.AvaliableLength());
					printf("this time proc data sum = %d\n", read_count);
					if (read_count > 0)
					{
						recv_data_.AdjustOffset(read_count);
						if ((recv_data_.OffsetPtr() - recv_data_.MemPtr()) > (recv_data_.Capacity() / 2))
						{
							int data_len = recv_data_.Length();
							memcpy(recv_data_.MemPtr(), recv_data_.OffsetPtr(), data_len);
							recv_data_.SetLength(data_len);
							recv_data_.Seek(0);
						}
					}
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
				//����ش�ʧ��Ҳ���ܵ�����
				//����ҲӦ�ô�����δ���������ٹ�����
				printf("read expect %d\n", error);
				Close();
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
		if (data->AvaliableLength() > 0) {
			int32_t n = 0;
			n = socket_.Send(data->OffsetPtr(), data->AvaliableLength(), 0);
			//�ڷ�����ģʽ��,send�����Ĺ��̽����ǽ����ݿ�����Э��ջ�Ļ���������,������������ÿռ䲻��,�������Ŀ���,
			//�������سɹ������Ĵ�С;�绺�������ÿռ�Ϊ0,�򷵻�-1,ͬʱ����errnoΪEAGAIN.
			if (n > 0) {
				send_count += n;
				data->AdjustOffset(n);
				if (data->AvaliableLength() == 0) {
					free_data_.push_back(data);
					it = send_data_.erase(it);
					printf("����һ������ɹ�\n");
				}
				else {
					printf("����̫��,�����ܿ�����%d���ݵ����ͻ���\n", send_count);
					EnableWrite(true);
					break;
				}
			}
			else if (n <= 0) {
				int error = ErrerCode;
#ifdef _MSC_VER
				if (error == WSAEINTR) {
					printf("���ͱ��жϣ��´λص��ٷ�\n");
					break;
				}
				else if (error == WSAEWOULDBLOCK) {
					printf("��������\n");
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
					printf( "write expect %d\n", error);
					break;
				}
			}
		}
	}
	if (send_data_.empty()) {
		printf("ȫ���ͳɹ�\n");
	}
	
}

void TcpConn::HandleError() {
	printf( "TcpConn HandleError\n");
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
	Buffer* data = NULL;
	if (free_data_.empty())
	{
		data = new Buffer(RECV_ONCV_SIZE);
	}
	else {
		free_data_.pop_back();
		data = free_data_.back();
		data->SetLength(0);
	}
	data->WriteBuff(buf, len);
	send_data_.push_back(data);
}

void TcpConn::Close() {
	conn_state_ = TcpConn::CONNSTATE_CLOSED;
	socket_.Close();
	Disconnected();
}