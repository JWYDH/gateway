#pragma once

#include <errno.h>
#include <vector>
#include <functional>
#include "../core/jw_thread.h"
#include "../core/jw_log.h"
#include "../core/jw_socket.h"
#include "../core/jw_buffer.h"
#include "../core/jw_ring_buffer.h"
#include "../core/jw_lock_queue.h"
#include "../core/jw_lock_free_queue.h"


namespace jw
{
class TcpClient
{
public:
	enum CONN_STATE
	{
		CONNSTATE_CLOSED,
		CONNSTATE_CONNECTING,
		CONNSTATE_CONNECTED
	};
	enum BUFF_SIZE
	{
		RECV_MAX_SIZE = 1024 * 1024 * 8,
		RECV_BUF_SIZE = 1024 * 1024 * 2,
		SEND_BUF_SIZE = 1024 * 1024 * 2
	};

public:
	TcpClient();
	virtual ~TcpClient();
	TcpClient &operator=(const TcpClient &) = delete;
private:
	void Connected();
	void Disconnected();

	void _read_thread_func();
	void _write_thread_func();
public:
	bool Start(const char *ip, const short port);
	void Stop();

	int ConnState() { return conn_state_; }
	void DoWrite(const char *buf, int32_t len);

	void OnConnected(std::function<void(TcpClient *)> connected_callback) { connected_callback_ = connected_callback; }
	void OnDisconnected(std::function<void(TcpClient *)> disconnected_callback) { disconnected_callback_ = disconnected_callback; }
	void OnRead(std::function<void(TcpClient *)> read_callback) { read_callback_ = read_callback; }

private:
	bool stoped_;
	thread_t thread_;
	thread_t write_thread_;

	socket_t socket_;
	int conn_state_;
	sockaddr_in local_addr_;
	sockaddr_in remote_addr_;

	RingBuf recv_data_;

	LockQueue<Buffer *> send_data_pending;
	std::list<Buffer *> send_data_;

	std::function<void(TcpClient *)> connected_callback_;
	std::function<void(TcpClient *)> disconnected_callback_;
	std::function<void(TcpClient *)> read_callback_;
};

} // namespace jw