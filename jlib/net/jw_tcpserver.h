#pragma once

#include <vector>
#include <map>
#include <functional>
#include <thread>
#include "jw_socket.h"
#include "jw_thread.h"
#include "jw_safe_queue.h"



class TcpConn;
class TcpServer {
public:
	TcpServer();
	virtual ~TcpServer();
	TcpServer& operator=(const TcpServer&) = delete;
	bool Initialize();
	void AddConvey(SOCKET fd, void *c);
	void DelConvey(SOCKET fd);
	void SetReadEvent(SOCKET fd, void *io, bool enable);
	void SetWriteEvent(SOCKET fd, void *io, bool enable);
	void Loop();
private:

#ifdef _MSC_VER
	fd_set fds_;
	fd_set fdreads_;
	fd_set fdwrites_;
	std::map<SOCKET, void*> select_conn_list_;
#else
	SOCKET listen_fd_;
#endif // _MSC_VER
public:
	union InterMsgData
	{
		struct
		{
			SOCKET fd_;
			void* ptr_;
		};
	};
	struct InterMsg 
	{
		int32_t msg_id_;
		InterMsgData msg_data_;

		InterMsg() {
			msg_id_ = 0;
		}
	};

	enum {
		gcAddClient = 1,	//增加一个新的客户端连接
		gcGWData = 2, //游戏服给客户端发消息
		gcGWClose = 3,	//游戏服主动关闭客户端
		gcChBro = 4,//频道广播消息
		gcChAdd = 5,//频道信息增加
		gcChDel = 6,//频道信息删除
		gcGWDisconn = 7,//游戏服断开连接
	};

	bool Start(const char *ip, const short port, int back_log=256);
	void Stop();
	bool PushInterMsg(InterMsg &msg);
	void ProcessInterMsg();
	void OnRecvSysMsg(InterMsg& msg);
	void OnNewSession(std::function<void(TcpConn*)> newsession_callback) { newsession_callback_ = newsession_callback; }
private:
	bool stoped_;
	SafeQueue<InterMsg> inter_msg_;
	SafeQueue<InterMsg> proc_inter_msg_;
	JwSocket socket_;
	JwSocket socket_pair_send_;
	JwSocket socket_pair_recv_;
	BaseThread accept_thread_;
	BaseThread work_thread_;
	std::map<SOCKET, TcpConn*> conn_list_;

	std::function<void(TcpConn*)> newsession_callback_;
};

