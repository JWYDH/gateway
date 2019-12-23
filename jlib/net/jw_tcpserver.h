#pragma once
#include <thread>
#include <vector>
#include <map>
#include <functional>
#include "jw_socket.h"
#include "../base/jw_thread.h"
#include "../base/jw_safe_queue.h"


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

#ifdef WIN32
	fd_set fds_;
	fd_set fdreads_;
	fd_set fdwrites_;
	std::map<SOCKET, void*> select_conn_list_;
#else
	SOCKET listen_fd_;
#endif
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
		gcAddClient = 1,	//����һ���µĿͻ�������
		gcGWData = 2, //��Ϸ�����ͻ��˷���Ϣ
		gcGWClose = 3,	//��Ϸ�������رտͻ���
		gcChBro = 4,//Ƶ���㲥��Ϣ
		gcChAdd = 5,//Ƶ����Ϣ����
		gcChDel = 6,//Ƶ����Ϣɾ��
		gcGWDisconn = 7,//��Ϸ���Ͽ�����
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

