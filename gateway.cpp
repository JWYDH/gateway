// gateway.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>

#include "depend/jw_tcpconn.h"
#include "depend/jw_tcpserver.h"

int main()
{
	JwSocket::Init();
	
#ifndef _MSC_VER		
	runing = true;
	//signal(SIGTERM, sigterm_handler);
	signal(SIGPIPE, SIG_IGN);
	//signal(SIGHUP, sighup_handler);
#endif

	TcpServer server;
	server.OnNewSession([](TcpConn* conn) {
		//OnConnected([this](TcpConn* conn) {
		//	sockaddr_in local_addr;
		//	sockaddr_in remote_addr;
		//	socket_.getLoaclAddr(&local_addr);
		//	socket_.getRemoteAddr(&remote_addr);
		//	printf( "OnConnected fd=%d, %s ========= %s\n", (int)socket, inet_ntoa(local_addr.sin_addr), inet_ntoa(local_addr.sin_addr));
		//});

		//OnDisconnected([this](TcpConn* conn) {
		//	sockaddr_in local_addr;
		//	sockaddr_in remote_addr;
		//	socket_.getLoaclAddr(&local_addr);
		//	socket_.getRemoteAddr(&remote_addr);
		//	printf( "OnDisconnected fd=%d, %s ====|==== %s\n", (int)socket, inet_ntoa(local_addr.sin_addr), inet_ntoa(local_addr.sin_addr));
		//});

		//OnRead([this](TcpConn* conn, char* buf, int32_t len) {
		//	sockaddr_in local_addr;
		//	sockaddr_in remote_addr;
		//	socket_.getLoaclAddr(&local_addr);
		//	socket_.getRemoteAddr(&remote_addr);
		//	printf( "OnDisconnected fd=%d, %s ====|==== %s\n", (int)socket, inet_ntoa(local_addr.sin_addr), inet_ntoa(local_addr.sin_addr));
		//});
	});
	server.Start("127.0.0.1", 8001);
	
	system("pause");
	server.Stop();
	system("pause");
	JwSocket::Clean();
	return 0;

}
