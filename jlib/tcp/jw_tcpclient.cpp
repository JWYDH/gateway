#include "jw_tcpclient.h"


TcpClient::TcpClient(){
	conn_state_ = TcpClient::CONNSTATE_CLOSED;

	OnConnected( [this](TcpClient* conn) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		
		printf( "OnConnected fd=%d, %s:%d ========= %s:%d\n", (int)socket_
			, inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port), inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
	});

	OnDisconnected([this](TcpClient* conn) {
		sockaddr_in local_addr;
		sockaddr_in remote_addr;
		socket_.getLoaclAddr(&local_addr);
		socket_.getRemoteAddr(&remote_addr);
		printf("OnDisconnected fd=%d, %s:%d ========= %s:%d\n", (int)socket_
			, inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port), inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
	});

	OnRead([this](TcpClient* conn, char* buf, int32_t len)->int32_t {
		DoWrite(buf, len);
		return len;
	});
}

TcpClient::~TcpClient() {
	for (auto& buf : free_data_){
		delete buf;
	}
	for (auto& buf : send_data_){
		delete buf;
	}
}

bool TcpClient::Initialize(){
	stoped_ = false;

	FD_ZERO(&fds_);
	FD_ZERO(&fdreads_);
	return true;
}

void TcpClient::AddConvey(SOCKET fd) {
	FD_SET(fd, &fds_);
	FD_SET(fd, &fdreads_);
}

void TcpClient::DelConvey(SOCKET fd) {
	FD_CLR(fd, &fds_);
	FD_CLR(fd, &fdreads_);
}

void TcpClient::Loop() {
	fd_set fds;
	fd_set fdr;
	memcpy(&fds, &fds_, sizeof(fds_));
	memcpy(&fdr, &fdreads_, sizeof(fdreads_));
	int   nRetAll = select(0/*window��Ϊ0*/, &fdr, NULL, NULL, NULL);
	if (nRetAll > 0) {
		for (uint32_t i = 0; i < fds.fd_count; ++i) {
			auto fd = fds.fd_array[i];
			if (FD_ISSET(fd, &fdr)) {
				HandleRead();
			}
		}
	}else if (nRetAll == 0){
#ifdef WIN32
		printf("select time out! %d\n", WSAGetLastError());
#else
		printf("select time out: %d\n", errno);
#endif // WIN32
	}else{
#ifdef WIN32
		printf("select error!%d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		
	}
}

bool TcpClient::Start(const char* ip, const short port) {
	if (!Initialize()) {
		printf("Initialize error\n");
		return false;
	}
	if (!socket_.Create()) {

#ifdef WIN32
		printf("creat error!%d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
	}
	auto thread_func = [this, ip, port]() {
		while (!stoped_){
			if (conn_state_ == TcpClient::CONNSTATE_CONNECTED){
				this->Loop();
			}else {
				if (!socket_.Create()) {
#ifdef WIN32
					printf("Disconnected error!%d\n", WSAGetLastError());
#else
					printf("Disconnected error!%d\n", errno);
#endif // WIN32

				}

				if (socket_.Connect(ip, port)) {
					Connected();
				}
				else {
					socket_.Close();
					printf("connnect fial\n");
					Sleep(1000);
				}
			}
		}
		socket_.Close();
	};
	thread_.Start(thread_func);

	auto write_thread_func = [this, ip, port]() {
		while (!stoped_)
		{
			if (conn_state_ != TcpClient::CONNSTATE_CONNECTED){
				Sleep(1);
			}
			if (this->HandleWrite()) {
				Sleep(1);
			}
		}
		socket_.Close();
	};
	write_thread_.Start(write_thread_func);
	return true;
}


void TcpClient::Stop(){
	stoped_ = true;
}

void TcpClient::Connected() {
	printf("Connect success \n");
	conn_state_ = TcpClient::CONNSTATE_CONNECTED;
	socket_.SetNoBlock();
	if (!socket_.SetRecvBufSize(RECV_BUF_SIZE)) {

#ifdef WIN32
		printf("set recv error!%d\n", WSAGetLastError());
#else
		printf("set recv error!%d\n", errno);
#endif // WIN32
	}
	if (!socket_.SetSendBufSize(SEND_BUF_SIZE)) {

#ifdef WIN32
		printf("set send error!%d\n", WSAGetLastError());
#else
		printf("set send error!%d\n", errno);
#endif // WIN32
	}
	connected_callback_(this);
	AddConvey((SOCKET)socket_);
}



void TcpClient::Disconnected() {
	DelConvey((SOCKET)socket_);
	printf("Disconnected success \n");
	conn_state_ = TcpClient::CONNSTATE_CLOSED;
	disconnected_callback_(this);
	socket_.Close();

}

void TcpClient::HandleRead() {
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
			MyMethod();
			Disconnected();
			break;
		}
		if (n < 0) {
			
#ifdef WIN32
			int error = WSAGetLastError();
			if (error == WSAEINTR) {
				continue;
			}
			else if (error == WSAEWOULDBLOCK) {
				if (count > 0)
				{
					printf("this time recv data sum = %d\n", count);
					MyMethod();

				}
				break;
			}
#else
			int error = errno;
			if (error == EINTR) {
				continue;
			}
			else if (error == EAGAIN || error == EWOULDBLOCK || error == EINPROGRESS) {
				break;
			}
#endif
			else {
				//����ҲӦ�ô�����δ���������ٹ�����
				printf("read expect %d\n", error);
				MyMethod();
				Disconnected();
				break;
			}
		}
	}
}

bool TcpClient::HandleWrite() {
	inter_msg_.append(send_data_);
	if (send_data_.empty()) {
		return true;
	}
	int32_t send_count = 0;
	for (auto it = send_data_.begin(); it != send_data_.end(); ) {
		auto &data = *it;
		int32_t n = 0;
		n = socket_.Send(data->OffsetPtr(), data->AvaliableLength());
		//�ڷ�����ģʽ��,send�����Ĺ��̽����ǽ����ݿ�����Э��ջ�Ļ���������,������������ÿռ䲻��,�������Ŀ���,
		//�������سɹ������Ĵ�С;�绺�������ÿռ�Ϊ0,�򷵻�-1,ͬʱ����errnoΪEAGAIN.
		if (n > 0) {
			send_count += n;
			data->AdjustOffset(n);
			if (data->AvaliableLength() == 0) {
				delete data;
				it = send_data_.erase(it);
				printf("����һ������ɹ�\n");
			}else {
				printf("����̫��,�����ܿ�����%d���ݵ����ͻ���\n", send_count);
				break;
			}
		}else if (n <= 0) {
#ifdef WIN32
			int error = WSAGetLastError();
			if (error == WSAEINTR) {
				printf("���ͱ��жϣ��´λص��ٷ�\n");
				break;
			}else if (error == WSAEWOULDBLOCK) {
				printf("��������\n");
				//break;
				return true;
			}
#else
			int error = errno;
			if (error == EINTR) {
				break;
			}else if (error == EAGAIN || error == EWOULDBLOCK) {
				break;
			}
#endif
			else {
				printf( "write expect %d\n", error);
				break;
			}
		}
	}
	if (send_data_.empty()) {
		printf("ȫ���ͳɹ�\n");
	}
	return false;
}

void TcpClient::HandleError() {
	printf( "TcpConn HandleError\n");
	Close();
}

void TcpClient::DoWrite(const char *buf, int32_t len) {
	if (stoped_){
		printf("PushInterMsgxx fail\n");
		return;
	}
	Buffer* data = new Buffer(RECV_ONCV_SIZE);
	data->WriteBuff(buf, len);
	inter_msg_.push(std::move(data));
}

void TcpClient::Close() {

}