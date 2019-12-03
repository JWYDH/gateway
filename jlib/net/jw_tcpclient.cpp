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
	for (auto& buf : free_data_)
	{
		delete buf;
	}
	for (auto& buf : send_data_)
	{
		delete buf;
	}
}

bool TcpClient::Initialize()
{
	stoped_ = false;

	FD_ZERO(&fds_);
	FD_ZERO(&fdreads_);
	FD_ZERO(&fdwrites_);

	return true;
}

void TcpClient::AddConvey(SOCKET fd) {

	FD_SET(fd, &fds_);
	FD_SET(fd, &fdreads_);


}

void TcpClient::DelConvey(SOCKET fd) {

	FD_CLR(fd, &fds_);
	FD_CLR(fd, &fdreads_);
	//FD_CLR(fd, &fdwrites_);

}

void TcpClient::SetWriteEvent(SOCKET fd, bool enable) {

	if (enable)
	{
		FD_SET(fd, &fdwrites_);
	}
	else
	{
		FD_CLR(fd, &fdwrites_);

	}
}

void TcpClient::Loop() {

	fd_set fds;
	fd_set fdr;
	fd_set fdw;
	memcpy(&fds, &fds_, sizeof(fds_));
	memcpy(&fdr, &fdreads_, sizeof(fdreads_));
	memcpy(&fdw, &fdwrites_, sizeof(fdwrites_));
	timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	int   nRetAll = select(0/*window可为0*/, &fdr, NULL, NULL, NULL /*&time*/);//若不设置超时则select为阻塞  
	if (nRetAll > 0) {
		for (uint32_t i = 0; i < fds.fd_count; ++i) {
			auto fd = fds.fd_array[i];
			
			if (FD_ISSET(fd, &fdr)) {
				//调用recv，接收数据。 
				HandleRead();
			}
			//if (FD_ISSET(fd, &fdw)) {
			//	//轮询检查有数据可写。 
			//	HandleWrite();
			//}
			//if (FD_ISSET(fd, &fde)) {
			//	ptr->HandleError();
			//}
		}

	}
	else if (nRetAll == 0)
	{
		//printf("select time out \n");
	}
	else
	{
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
		while (!stoped_)
		{
			if (conn_state_ == TcpClient::CONNSTATE_CONNECTED)
			{
				this->Loop();
			}else {
				if (socket_.Connect(ip, port)) {
					Connected();
				}
				else {
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
			if (conn_state_ == TcpClient::CONNSTATE_CONNECTED)
			{
				if (this->HandleWrite()) {
					//printf("write sleep 111\n");
					Sleep(1);
				}
			}
			else {
					//printf("write sleep 222\n");
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
//	if (!socket_.Create()) {
//#ifdef WIN32
//		printf("Disconnected error!%d\n", WSAGetLastError());
//#else
//		printf("Disconnected error!%d\n", errno);
//#endif // WIN32
//
//	}
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
			//这里应该处理完未处理数据再关连接
			printf("close normal\n");
			//Close();
			Disconnected();
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
				//第二次写？？？断了连接 管道破裂的异常 SIGPIPE,
				//这里也应该处理完未处理数据再关连接
				printf("read expect %d\n", error);
				//Close();
				Disconnected();
				break;
			}
		}
	}
}

bool TcpClient::HandleWrite() {
	//SetWriteEvent((SOCKET)socket_, false);

	
	inter_msg_.append(send_data_);
	if (send_data_.empty()) {
		return true;
	}

	int32_t send_count = 0;
	for (auto it = send_data_.begin(); it != send_data_.end(); ) {
		auto &data = *it;
		if (data->AvaliableLength() > 0) {
			int32_t n = 0;
			n = socket_.Send(data->OffsetPtr(), data->AvaliableLength(), 0);
			//在非阻塞模式下,send函数的过程仅仅是将数据拷贝到协议栈的缓存区而已,如果缓存区可用空间不够,则尽能力的拷贝,
			//立即返回成功拷贝的大小;如缓存区可用空间为0,则返回-1,同时设置errno为EAGAIN.
			if (n > 0) {
				send_count += n;
				data->AdjustOffset(n);
				if (data->AvaliableLength() == 0) {
					//data->SetLength(0);
					//data->Seek(0);
					//free_data_.push_back(data);
					delete data;
					it = send_data_.erase(it);
					printf("发送一个大包成功\n");
				}
				else {
					printf("数据太大,尽可能拷贝了%d数据到发送缓冲\n", send_count);
					//SetWriteEvent((SOCKET)socket_, true);
					break;
				}
			}
			else if (n <= 0) {
				int error = ErrerCode;
#ifdef _MSC_VER
				if (error == WSAEINTR) {
					printf("发送被中断，下次回调再发\n");
					break;
				}
				else if (error == WSAEWOULDBLOCK) {
					printf("缓冲区满\n");
					//break;
					return true;
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
		printf("全发送成功\n");
		//return true;
	}
	return false;
}

void TcpClient::HandleError() {
	printf( "TcpConn HandleError\n");
	Close();
}

//减少memcpy，用列表保存要发送的数据包
//避免发送太快阻塞其他连接发送，每个发送数据包设置发送缓冲区大小
void TcpClient::DoWrite(const char *buf, int32_t len) {
	if (stoped_)
	{
		//停止后不能接受消息
		printf("PushInterMsgxx fail\n");
		return;
	}
	//if (send_data_.empty()) {
	//	SetWriteEvent((SOCKET)socket_, true);
	//}
	//auto it = send_data_.rbegin();
	//if (it != send_data_.rend())
	//{
	//	auto data = (*it);
	//	if (data->AvaliableCapacity() >= len)
	//	{
	//		data->WriteBuff(buf, len);
	//		return;
	//	}
	//}
	Buffer* data = NULL;
	//if (free_data_.empty())
	//{
		data = new Buffer(RECV_ONCV_SIZE);
	//}
	//else {
	//	free_data_.pop_back();
	//	data = free_data_.back();
	//	data->SetLength(0);
	//}
	data->WriteBuff(buf, len);
	//send_data_.push_back(data);


	inter_msg_.push(std::move(data));
}

void TcpClient::Close() {

}