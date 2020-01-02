#pragma once
#include <errno.h>
#include <assert.h>
#include <vector>
#include <functional>
#include "../core/jw_thread.h"
#include "../core/jw_log.h"
#include "../core/jw_socket.h"
#include "../core/jw_poller.h"
#include "../core/jw_ring_buffer.h"
#include "../core/jw_lock_queue.h"
#include "../core/jw_lock_free_queue.h"

namespace jw
{

struct TcpConn
{
	socket_t socket_;
	int conn_state_;
	sockaddr_in local_addr_;
	sockaddr_in remote_addr_;

	RingBuf recv_data_;
	std::list<RingBuf *> send_data_;
	LockQueue<RingBuf *> send_data_pending;
	void DoWrite(const char *buf, int32_t len)
	{
		RingBuf *data = new RingBuf();
		data->write(buf, len);
		send_data_pending.push(std::move(data));
	}
};

class TcpServer
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
	TcpServer();
	virtual ~TcpServer();
	TcpServer &operator=(const TcpServer &) = delete;

private:
	void _server_func(void *);

public:
	bool Start(const char *ip, const short port);
	void Stop();

	void OnConnected(std::function<void(TcpConn *)> connected_callback) { connected_callback_ = connected_callback; }
	void OnDisconnected(std::function<void(TcpConn *)> disconnected_callback) { disconnected_callback_ = disconnected_callback; }
	void OnRead(std::function<void(TcpConn *, RingBuf &)> read_callback) { read_callback_ = read_callback; }

private:
	bool stoped_;
	thread_t server_thread_;

	socket_t epoll_fd_;
	socket_t listen_fd_;

	std::vector<TcpConn *> connects_;

	std::function<void(TcpConn *)> connected_callback_;
	std::function<void(TcpConn *)> disconnected_callback_;
	std::function<void(TcpConn *, RingBuf &)> read_callback_;
};

} // namespace jw