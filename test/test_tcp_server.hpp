#pragma once
#include "net/jw_tcpconn.h"
#include "net/jw_tcpserver.h"

int test_tcpserver()
{

#ifdef WIN32
	JwSocket::Init();
#endif 

	TcpServer server;
	server.OnNewSession([](TcpConn* conn) {

	});
	server.Start("0.0.0.0", 8000);

	system("pause");

	server.Stop();
	return 0;
}