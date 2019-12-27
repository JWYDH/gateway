#pragma once
#include <stdint.h>
#include <vector>
#include <functional>
#include "jw_socket.h"
#include "jw_buffer.h"


class TcpServer;

#define RECV_ONCV_SIZE 1024
#define RECV_MAX_SIZE 1024 * 1024 * 8
#define RECV_BUF_SIZE 1024 * 1024
#define SEND_BUF_SIZE 1024 * 1024
class TcpConn{
public:
	enum CONN_STATE {
		CONNSTATE_CLOSED,
		CONNSTATE_CONNECTED
	};
public:
	TcpConn(TcpServer* events);
	virtual ~TcpConn();
	TcpConn& operator=(const TcpConn&) = delete;
	void EnableWrite(bool enable);
	virtual void HandleRead();
	virtual void HandleWrite();
	virtual void HandleError();
private:
	TcpServer* events_;
public:
	void Connected();
	void Disconnected();
	int GetConnState() { return conn_state_; }
	JwSocket& GetSocket() { return socket_; }

	void DoWrite(const char *buf, int32_t len);
	void Close();

	void OnConnected(std::function<void(TcpConn*)> connected_callback) { connected_callback_ = connected_callback; }
	void OnDisconnected(std::function<void(TcpConn*)> disconnected_callback) { disconnected_callback_ = disconnected_callback; }
	void OnRead(std::function<int32_t(TcpConn*, char*, int32_t)> read_callback) { read_callback_ = read_callback; }
private:
	int conn_state_;
	JwSocket socket_;

	
	Buffer recv_data_;
	std::vector<Buffer*> free_data_;
	std::vector<Buffer*> send_data_;
	std::function<void(TcpConn*)> connected_callback_;
	std::function<void(TcpConn*)> disconnected_callback_;
	std::function<int32_t(TcpConn*, char*, int32_t)> read_callback_;
};


