#pragma once

#include <errno.h>
#include <vector>
#include <functional>
#include "../core/jw_thread.h"
#include "../core/jw_log.h"
#include "../core/jw_socket.h"
#include "../core/jw_buffer.h"
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
		CONNSTATE_CONNECTED
	};
	enum BUFF_SIZE
	{
		RECV_ONCV_SIZE = 1024,
		RECV_MAX_SIZE = 1024 * 1024 * 8,
		RECV_BUF_SIZE = 1024 * 1024,
		SEND_BUF_SIZE = 1024 * 1024
	};

public:
	TcpClient();
	virtual ~TcpClient();
	TcpClient &operator=(const TcpClient &) = delete;

private:
	fd_set fds_;
	fd_set fdreads_;
	fd_set fdwrites_;

private:
	virtual void HandleRead();

	void MyMethod()
	{
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

	virtual bool HandleWrite();
	virtual void HandleError();

	void Connected();
	void Disconnected();

public:
	bool Start(const char *ip, const short port);
	void Stop();

	int ConnState() { return conn_state_; }
	void DoWrite(const char *buf, int32_t len);

	void OnConnected(std::function<void(TcpClient *)> connected_callback) { connected_callback_ = connected_callback; }
	void OnDisconnected(std::function<void(TcpClient *)> disconnected_callback) { disconnected_callback_ = disconnected_callback; }
	void OnRead(std::function<int32_t(TcpClient *, char *, int32_t)> read_callback) { read_callback_ = read_callback; }

private:
	bool stoped_;
	thread_t thread_;
	thread_t write_thread_;

	socket_t socket_;
	int conn_state_;
	sockaddr_in local_addr_;
	sockaddr_in remote_addr_;

	Buffer recv_data_;
	LockQueue<Buffer *> write_msg_;
	std::list<Buffer *> send_data_;

	std::function<void(TcpClient *)> connected_callback_;
	std::function<void(TcpClient *)> disconnected_callback_;
	std::function<int32_t(TcpClient *, char *, int32_t)> read_callback_;
};

} // namespace jw