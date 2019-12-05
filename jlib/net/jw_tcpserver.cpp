#include "jw_tcpserver.h"
#include "jw_tcpconn.h"



TcpServer::TcpServer()
{
}

TcpServer::~TcpServer(){
}

bool TcpServer::Initialize()
{
	stoped_ = false;
#ifdef WIN32
	FD_ZERO(&fds_);
	FD_ZERO(&fdreads_);
	FD_ZERO(&fdwrites_);
#else
	listen_fd_ = epoll_create1(EPOLL_CLOEXEC);
#endif
	return true;
}

void TcpServer::AddConvey(SOCKET fd, void *c) {
#ifdef WIN32
	auto it = select_conn_list_.find(fd);
	if (it != select_conn_list_.end())
	{
		printf("EventLoop::AddConvey error. fd = %d", fd);
		assert(true);
		return;
	}
	FD_SET(fd, &fds_);
	FD_SET(fd, &fdreads_);
	select_conn_list_[fd] = c;
#else
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = event;
	ev.data.ptr = c;
	int r = epoll_ctl(listen_fd_, EPOLL_CTL_ADD, fd, &ev);
	if (r) {
		printf( "epoll_ctl add failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif //

}

void TcpServer::DelConvey(SOCKET fd) {
#ifdef WIN32
	auto it = select_conn_list_.find(fd);
	if (it == select_conn_list_.end())
	{
		printf("EventLoop::DelConvey error. fd = %d", fd);
		assert(true);
		return;
	}
	FD_CLR(fd, &fds_);
	FD_CLR(fd, &fdreads_);
	FD_CLR(fd, &fdwrites_);
	select_conn_list_.erase(it);
#else
	int r = epoll_ctl(listen_fd_, EPOLL_CTL_DEL, fd, nullptr);
	if (r) {
		printf( "epoll_ctl del failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif //
}

void TcpServer::SetReadEvent(SOCKET fd, void *io, bool enable) {
#ifdef WIN32
	if (enable)
	{
		FD_SET(fd, &fdreads_);
	}
	else
	{
		FD_CLR(fd, &fdreads_);

	}
#else
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = event;
	ev.data.ptr = io;
	int r = epoll_ctl(listen_fd_, EPOLL_CTL_MOD, fd, &ev);
	if (r) {
		printf( "epoll_ctl mod failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif //
}

void TcpServer::SetWriteEvent(SOCKET fd, void *io, bool enable) {
#ifdef WIN32
	if (enable)
	{
		FD_SET(fd, &fdwrites_);
	}
	else
	{
		FD_CLR(fd, &fdwrites_);
		
	}
#else
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = event;
	ev.data.ptr = io;
	int r = epoll_ctl(listen_fd_, EPOLL_CTL_MOD, fd, &ev);
	if (r) {
		printf( "epoll_ctl mod failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif //
}

void TcpServer::Loop() {
#ifdef WIN32
	fd_set fds;
	fd_set fdr;
	fd_set fdw;
	memcpy(&fds, &fds_, sizeof(fds_));
	memcpy(&fdr, &fdreads_, sizeof(fdreads_));
	memcpy(&fdw, &fdwrites_, sizeof(fdwrites_));
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	int   nRetAll = select(0/*window可为0*/, &fdr, &fdw, NULL, NULL /*&time*/);//若不设置超时则select为阻塞  
	if (nRetAll > 0) {
		std::vector<SOCKET> close_list;
		for (uint32_t i = 0; i < fds.fd_count; ++i) {
			auto fd = fds.fd_array[i];
			auto it = select_conn_list_.find(fd);
			if (it == select_conn_list_.end())
			{
				printf("EventLoop::not find error. fd = %d", fd);
				assert(true);
			}
			auto ptr = it->second;
			if (!ptr) {
				printf( "ptr is null fd %d is null\n", fd);
				assert(true);
			}
			if (this == it->second) {
				if (FD_ISSET(fd, &fdr)) {
					assert(fd == (SOCKET)socket_pair_recv_);
					char buf[1024] = { 0 };
					int n = 0;
					do {
						n = socket_pair_recv_.Recv(buf, sizeof(buf));
						if (n == 0) {
							socket_pair_recv_.Close();
						}
					} while (n >= sizeof(buf));
					ProcessInterMsg();
					
				}
				continue;
			}
			
			if (FD_ISSET(fd, &fdr)) {
				//调用recv，接收数据。 
				((TcpConn*)ptr)->HandleRead();
			}
			if (FD_ISSET(fd, &fdw)) {
				//轮询检查有数据可写。 
				((TcpConn*)ptr)->HandleWrite();
			}
			//if (FD_ISSET(fd, &fde)) {
			//	ptr->HandleError();
			//}
			if (((TcpConn*)ptr)->GetConnState() == TcpConn::CONNSTATE_CLOSED) {
				close_list.push_back(fd);
			}
			
		}
		for (auto &fd : close_list)
		{
			DelConvey(fd);
			auto it = conn_list_.find(fd);
			if (it == conn_list_.end())
			{
				printf("close_list::dell error. fd = %d", fd);
				assert(true);
				return;
			}
			delete it->second;
			conn_list_.erase(it);
		}
	}
	else if (nRetAll == 0)
	{
		printf("select time out \n");
	}
	else
	{
		printf("select error!%d\n", WSAGetLastError());
	}

#else
	while (true) {
		const static int kMaxEvents = 20;
		struct epoll_event evs[kMaxEvents];
		int n = epoll_wait(listen_fd_, evs, kMaxEvents, 0);
		if (n == -1) {
			if (ErrerCode == EINTR) {
				continue;
			}
			else {
				printf( "epoll_wait error %d %s", ErrerCode, strerror(ErrerCode));
				break;
			}
		}
		for (int i = 0; i < n; i++) {
			void* ptr = evs[i].data.ptr;
			TcpConn* io = (TcpConn*)ptr;
			int ev = evs[i].events;
			if (ev & (EPOLLERR | EPOLLHUP)) {
				printf( "epoll_wait triger %s %s ", ev | EPOLLERR ? "EPOLLERR" : "", ev | EPOLLERR ? "EPOLLHUP" : "");
				io->HandleError();
				continue;
			}
			if (ev & EPOLLIN) {
				io->HandleRead();
			}
			if (ev & EPOLLOUT) {
				io->HandleWrite();
			}
		}
	}
#endif //

}

bool TcpServer::Start(const char *ip, const short port, int back_log/*=256*/){
	if (!Initialize()) {
		printf("Initialize error\n");
		return false;
	}
	if (!socket_.Create()) {
#ifdef WIN32
		printf("create error: %d\n", WSAGetLastError());
#else
		printf("create error: %d\n", errno);
#endif // WIN32

		return false;
	}
	if (!socket_.Bind(ip, port)){
		socket_.Close();

#ifdef WIN32
		printf("bind error: %d\n", WSAGetLastError());
#else
		printf("bind error: %d\n", errno);
#endif // WIN32
		return false;
	}
	if (!socket_.Listen(back_log)) {
		socket_.Close();
#ifdef WIN32
		printf("listen error: %d\n", WSAGetLastError());
#else
		printf("listen error: %d\n", errno);
#endif // WIN32

		return false;
	}
	if (!socket_pair_send_.Create()) {
		socket_.Close();
#ifdef WIN32
		printf("create socket_pair_send_ error: %d\n", WSAGetLastError());
#else
		printf("create socket_pair_send_ error: %d\n", errno);
#endif // WIN32

		return false;
	}
	if (!socket_pair_recv_.Create()) {
		socket_.Close();
		socket_pair_send_.Close();
#ifdef WIN32
		printf("create socket_pair_send_ error: %d\n", WSAGetLastError());
#else
		printf("create socket_pair_send_ error: %d\n", errno);
#endif // WIN32
		return false;
	}
	if (!socket_pair_send_.Connect(ip, port))
	{
		socket_.Close();
		socket_pair_send_.Close();
		socket_pair_recv_.Close();
#ifdef WIN32
		printf("Connect fail error: %d\n", WSAGetLastError());
#else
		printf("Connect fail error: %d\n", errno);
#endif // WIN32

		return false;
	}
	if (!socket_.Accept(socket_pair_recv_))
	{
		socket_.Close();
		socket_pair_send_.Close();
		socket_pair_recv_.Close();
#ifdef WIN32
		printf("Accept fail error: %d\n", WSAGetLastError());
#else
		printf("Accept fail error: %d\n", errno);
#endif // WIN32

		return false;
	}
	socket_pair_send_.SetNoDelay();
	AddConvey((SOCKET)socket_pair_recv_, this);
	auto accept_thread_func = [this]() { 
		printf( "accept_thread_func\n"); 
		JwSocket socket;
		while (socket_.Accept(socket))
		{
			if (stoped_) {
				socket.Close();
				break;
			}else {
				//创建客户端连接类
				TcpConn* conn = new TcpConn(this);
				sockaddr_in local_addr;
				socket_.getLoaclAddr(&local_addr);
				socket.setLoaclAddr(&local_addr);
				conn->GetSocket() = socket;//默认拷贝构造

				if (newsession_callback_){
					newsession_callback_(conn);
				}
				

				InterMsg msg;
				msg.msg_id_ = gcAddClient;
				msg.msg_data_.fd_ = (SOCKET)socket;
				msg.msg_data_.ptr_ = conn;
				PushInterMsg(msg);
			}
		}
		socket_.Close();
	};
	accept_thread_.Start(accept_thread_func);

	//auto engine_thread_func = [this]() {
	//	printf( "engine_thread_func\n");
	//	while (!stoped_)
	//	{
	//		this->Loop();
	//	}
	//	socket_pair_recv_.Close();
	//};
	//work_thread_.Start(engine_thread_func);
	return true;
}


void TcpServer::Stop()
{
	stoped_ = true;
	//防止accept阻塞线程退出
	SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in svraddr;
	socket_.getLoaclAddr(&svraddr);
	::connect(fd, (struct sockaddr*)&svraddr, sizeof(svraddr));
	//等待accept线程退出
	accept_thread_.waitFor();
	//关闭连接唤醒select,防止阻塞,并处理所有内部消息
	socket_pair_send_.Close();
	work_thread_.waitFor();
}

bool TcpServer::PushInterMsg(InterMsg &msg)
{
	if (stoped_)
	{
		//停止后不能接受消息
		printf( "PushInterMsgxx fail\n");
		return false;
	}
	inter_msg_.push(std::move(msg));
	socket_pair_send_.Send("", 1);
	return true;
}

void TcpServer::ProcessInterMsg()
{
	std::list<InterMsg> items;
	items.clear();
	if (inter_msg_.swap(items)) {
		for (auto &msg : items){
			OnRecvSysMsg(msg);
		}
	}
}

void TcpServer::OnRecvSysMsg(InterMsg& msg) {
	switch (msg.msg_id_) {
	case gcAddClient: {
		SOCKET fd = msg.msg_data_.fd_;
		TcpConn* conn = (TcpConn*)msg.msg_data_.ptr_;
		conn->Connected();
		auto it = conn_list_.find(fd);
		if (it != conn_list_.end())
		{
			printf("EventLoop::AddConvey error. fd = %d", fd);
			return;
		}
		conn_list_[fd] = conn;
		AddConvey(fd, conn);
		break;
	}
	case gcGWData: {
		//Client* cli = cli_obj_mgr_->get(msg.data_.index_);
		//if (cli) {
		//	auto dp = msg.data_.packet_;
		//	cli->OnGameWorldRecv(dp->getOffsetPtr(), dp->getAvaliableLength());
		//}
		//gw_cli_->FreeBackUserDataPacket(msg.data_.packet_);
		break;
	}
	case gcGWClose: {
		//Client* cli = cli_obj_mgr_->get(msg.data_.index_);
		//if (cli) {
		//	cli->Close();
		//}
		break;
	}
	case gcChAdd: {
		//uint64_t key = MAKEINT64(msg.data_.channel_, msg.data_.para_);
		//channel_indexs_map_[key].insert(msg.data_.idx_);
		break;
	}
	case gcChDel: {
		//uint64_t key = MAKEINT64(msg.data_.channel_, msg.data_.para_);
		//channel_indexs_map_[key].erase(msg.data_.idx_);
		break;
	}
	case gcChBro: {
		//uint64_t key = MAKEINT64(msg.data_.b_channel_, msg.data_.b_para_);
		//auto& list = channel_indexs_map_[key];
		//for (auto idx : list) {
		//	Client* cli = cli_obj_mgr_->get(idx);
		//	if (cli) {
		//		auto dp = msg.data_.packet_;
		//		cli->OnGameWorldRecv(dp->getOffsetPtr(), dp->getAvaliableLength());
		//	}
		//}
		//gw_cli_->FreeBackUserDataPacket(msg.data_.dp_);
		break;
	}
	case gcGWDisconn: {
		//CloseAllClient();
		break;
	}
	}
}


