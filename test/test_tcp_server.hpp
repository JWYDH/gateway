#pragma once
#include "tcp/jw_tcpserver.h"

int test_tcpserver()
{
	jw::TcpServer server;
	server.OnRead([](jw::TcpConn* conn, jw::RingBuf &buf) {

	});
	server.Start("0.0.0.0", 8000);

	getchar();

	server.Stop();
	return 0;
}