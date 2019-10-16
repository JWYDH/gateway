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

	OnRead([this](TcpConn* conn, char* buf, int32_t len)->bool {
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
	for (auto& buf : recv_data_)
	{
		delete buf;
	}
	for (auto& buf : send_data_)
	{
		delete buf;
	}
}

void TcpConn::EnableWrite(bool enable) {
	events_->SetWriteEvent((SOCKET)socket_, this, true);
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
	int32_t count = 0;
	for (;;) {
		int32_t n = 0;
		Buffer* data = NULL;
		if (free_data_.empty())
		{
			data = new Buffer(RECV_ONCV_SIZE);
			n = socket_.Recv(data->MemPtr(), data->Capacity(), 0);
			if (n > 0) {
				count += n;
				data->SetLength(n);
				recv_data_.push_back(data);
				continue;
			}else {
				free_data_.push_back(data);
			}
		}else {
			data = free_data_.back();
			data->SetLength(0);
			n = socket_.Recv(data->MemPtr(), data->Capacity(), 0);
			if (n > 0) {
				count += n;
				data->SetLength(n);
				free_data_.pop_back();
				recv_data_.push_back(data);
				continue;
			}
		}

		if (n == 0) {
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
					for (auto it = recv_data_.begin(); it != recv_data_.end();++it)
					{
						read_callback_(this, (*it)->OffsetPtr(), (*it)->Length());
						free_data_.push_back((*it));
						//recv_data_.erase(it);
					}
					recv_data_.clear();
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
				Close();
				printf( "read expect %d\n", error);
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
					free_data_.push_back(data);
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
					printf( "write expect %d\n", error);
					break;
				}
			}
		}
	}
	if (send_data_.empty()) {
		printf("ȫ���ͳɹ�");
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