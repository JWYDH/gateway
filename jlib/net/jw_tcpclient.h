#pragma once

#include <vector>
#include <functional>
#include "jw_socket.h"
#include "jw_buffer.h"
#include "jw_thread.h"
#include "jw_safe_queue.h"

class TcpServer;

#define RECV_ONCV_SIZE 1024
#define RECV_MAX_SIZE 1024 * 1024 * 8
#define RECV_BUF_SIZE 1024 * 1024
#define SEND_BUF_SIZE 1024 * 1024
class TcpClient{
public:
	enum CONN_STATE {
		CONNSTATE_CLOSED,
		CONNSTATE_CONNECTED
	};
public:
	TcpClient();
	virtual ~TcpClient();
	TcpClient& operator=(const TcpClient&) = delete;
	bool Initialize();
	void AddConvey(SOCKET fd);
	void DelConvey(SOCKET fd);
	void SetWriteEvent(SOCKET fd, bool enable);
	void Loop();
private:
	fd_set fds_;
	fd_set fdreads_;
	fd_set fdwrites_;


public:
	virtual void HandleRead();
	virtual bool HandleWrite();
	virtual void HandleError();

	bool Start(const char* ip, const short port);
	void Stop();
public:
	void Connected();
	void Disconnected();
	int GetConnState() { return conn_state_; }
	JwSocket& GetSocket() { return socket_; }
	void DoWrite(const char *buf, int32_t len);
	void Close();

	void OnConnected(std::function<void(TcpClient*)> connected_callback) { connected_callback_ = connected_callback; }
	void OnDisconnected(std::function<void(TcpClient*)> disconnected_callback) { disconnected_callback_ = disconnected_callback; }
	void OnRead(std::function<int32_t(TcpClient*, char*, int32_t)> read_callback) { read_callback_ = read_callback; }
private:
	int conn_state_;
	JwSocket socket_;
	bool stoped_;
	BaseThread thread_;
	Buffer recv_data_;
	BaseThread write_thread_;
	SafeQueue<Buffer*> inter_msg_;
	//std::vector<Buffer*> proc_inter_msg_;

	std::vector<Buffer*> free_data_;
	std::vector<Buffer*> send_data_;
	std::function<void(TcpClient*)> connected_callback_;
	std::function<void(TcpClient*)> disconnected_callback_;
	std::function<int32_t(TcpClient*, char*, int32_t)> read_callback_;
};


