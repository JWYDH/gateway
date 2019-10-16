#include "../stdafx.h"
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
	////create socket pair
	//sockaddr_in pairAddr;
	//memset(&pairAddr, 0, sizeof(pairAddr));
	//pairAddr.sin_family = AF_INET;
	//pairAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//my_addr.sin_port = 0;
	//int acpt = socket(AF_INET, SOCK_STREAM, 0);
	//_sockpair[0] = socket(AF_INET, SOCK_STREAM, 0);
	//_sockpair[1] = socket(AF_INET, SOCK_STREAM, 0);
	//if (_sockpair[0] == -1 || _sockpair[1] == -1 || acpt == -1)
	//{
	//	LCF("ZSummerImpl::initialize[this0x" << this << "] bind sockpair socket error. ");
	//	return false;
	//}
	//if (::bind(acpt, (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
	//{
	//	LCF("EventLoop::initialize[this0x" << this << "] bind sockpair socket error. ");
	//	::close(acpt);
	//	::close(_sockpair[0]);
	//	::close(_sockpair[1]);
	//	return false;
	//}
	//if (listen(acpt, 1) == -1)
	//{
	//	LCF("EventLoop::initialize[this0x" << this << "] listen sockpair socket error. ");
	//	::close(acpt);
	//	::close(_sockpair[0]);
	//	::close(_sockpair[1]);
	//	return false;
	//}
	//socklen_t len = sizeof(pairAddr);
	//if (getsockname(acpt, (sockaddr*)&pairAddr, &len) != 0)
	//{
	//	LCF("EventLoop::initialize[this0x" << this << "] getsockname sockpair socket error. ");
	//	::close(acpt);
	//	::close(_sockpair[0]);
	//	::close(_sockpair[1]);
	//	return false;
	//}

	//if (::connect(_sockpair[0], (sockaddr*)&pairAddr, sizeof(pairAddr)) == -1)
	//{
	//	LCF("EventLoop::initialize[this0x" << this << "] connect sockpair socket error. ");
	//	::close(acpt);
	//	::close(_sockpair[0]);
	//	::close(_sockpair[1]);
	//	return false;
	//}
	//_sockpair[1] = accept(acpt, (sockaddr*)&pairAddr, &len);
	//if (_sockpair[1] == -1)
	//{
	//	LCF("EventLoop::initialize[this0x" << this << "] accept sockpair socket error. ");
	//	::close(acpt);
	//	::close(_sockpair[0]);
	//	::close(_sockpair[1]);
	//	return false;
	//}
	//::close(acpt);

	//setNonBlock(_sockpair[0]);
	//setNonBlock(_sockpair[1]);
	//setNoDelay(_sockpair[0]);
	//setNoDelay(_sockpair[1]);
	//setEvent(_sockpair[1], 0);
#ifdef _MSC_VER
	FD_ZERO(&fds_);
	FD_ZERO(&fdreads_);
	FD_ZERO(&fdwrites_);
	
#else
	listen_fd_ = epoll_create1(EPOLL_CLOEXEC);
#endif // _MSC_VER
	return true;
}

void TcpServer::AddConvey(SOCKET fd, void *c) {
#ifdef _MSC_VER
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
		fprintf(stderr, "epoll_ctl add failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif // _MSC_VER

}

void TcpServer::DelConvey(SOCKET fd) {
#ifdef _MSC_VER
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
		fprintf(stderr, "epoll_ctl del failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif // _MSC_VER
}

void TcpServer::SetReadEvent(SOCKET fd, void *io, bool enable) {
#ifdef _MSC_VER
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
		fprintf(stderr, "epoll_ctl mod failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif // _MSC_VER
}

void TcpServer::SetWriteEvent(SOCKET fd, void *io, bool enable) {
#ifdef _MSC_VER
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
		fprintf(stderr, "epoll_ctl mod failed %d %s\n", ErrerCode, strerror(ErrerCode));
	}
#endif // _MSC_VER
}


void TcpServer::Loop() {
#ifdef _MSC_VER
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
		for (int i = 0; i < fds.fd_count; ++i) {
			auto fd = fds.fd_array[i];
			auto it = select_conn_list_.find(fd);
			if (it == select_conn_list_.end())
			{
				printf("EventLoop::not find error. fd = %d", fd);
				assert(true);
			}
			auto ptr = it->second;
			if (!ptr) {
				fprintf(stderr, "ptr is null fd %d is null\n", fd);
				assert(true);
			}
			if (this == it->second) {
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
				continue;
			}
			else {
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
			conn_list_.erase(it);
		}
	}
	else if (nRetAll == 0)
	{
		//printf("select time out \n");
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
				fprintf(stderr, "epoll_wait error %d %s", ErrerCode, strerror(ErrerCode));
				break;
			}
		}
		for (int i = 0; i < n; i++) {
			void* ptr = evs[i].data.ptr;
			TcpConn* io = (TcpConn*)ptr;
			int ev = evs[i].events;
			if (ev & (EPOLLERR | EPOLLHUP)) {
				fprintf(stdout, "epoll_wait triger %s %s ", ev | EPOLLERR ? "EPOLLERR" : "", ev | EPOLLERR ? "EPOLLHUP" : "");
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
#endif // _MSC_VER

}

bool TcpServer::Start(const char *ip, const short port, int back_log/*=256*/){
	if (!Initialize()) {
		fprintf(stdout, "Initialize error\n");
		return false;
	}
	if (!socket_.Create()) {
		fprintf(stderr, "create %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	if (!socket_pair_send_.Create()) {
		fprintf(stderr, "create socket_pair_send_ %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	if (!socket_pair_recv_.Create()) {
		fprintf(stderr, "create socket_pair_recv_ %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	if (!socket_.Bind(ip, port)){
		fprintf(stderr, "bind %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	if (!socket_.Listen(back_log)) {
		fprintf(stderr, "listen %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	if (!socket_pair_send_.Connect(ip, port))
	{
		fprintf(stderr, "Connect fail11 %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	if (!socket_.Accept(socket_pair_recv_))
	{
		fprintf(stderr, "Accept fail22 %d %s\n", ErrerCode, strerror(ErrerCode));
		return false;
	}
	AddConvey((SOCKET)socket_pair_recv_, this);
	auto accept_thread_func = [this]() { 
		fprintf(stderr, "accept_thread_func\n"); 
		while (!stoped_)
		{
			this->Accept();
		}
		socket_.Close();
	};
	accept_thread_.Start(accept_thread_func);

	auto engine_thread_func = [this]() {
		fprintf(stderr, "engine_thread_func\n");
		while (!stoped_)
		{
			this->Loop();
		}
		
		socket_pair_recv_.Close();
	};
	work_thread_.Start(engine_thread_func);
	return true;
}


void TcpServer::Stop()
{
	stoped_ = true;
	socket_pair_send_.Close();
	
	//防止accept阻塞线程退出
	//SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
	//struct sockaddr_in svraddr;
	//socket_.getLoaclAddr(&svraddr);
	//::connect(fd, (struct sockaddr*)&svraddr, sizeof(svraddr));
}

void TcpServer::Accept()
{
	//创建客户端连接类
	TcpConn* conn = new TcpConn(this);
	//创建连接类失败，则关闭套接字
	if (!conn)
	{
		fprintf(stdout, "not set on new tcp session\n");
		return;
	}

	JwSocket &socket = conn->GetSocket();
	if (!socket_.Accept(socket))
	{
		fprintf(stdout, "socket_.Accept fail %d\n", ErrerCode);
		return;
	}
	
	InterMsg msg;
	msg.msg_id_ = gcAddClient;
	msg.msg_data_.fd_ = (SOCKET)socket;
	msg.msg_data_.ptr_ = conn;
	PushInterMsg(msg);
}

void TcpServer::OnConnClose(TcpConn* conn)
{

}

bool TcpServer::PushInterMsg(InterMsg &msg)
{
	if (stoped_)
	{
		fprintf(stdout, "PushInterMsgxx fail\n");
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


