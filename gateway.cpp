// gateway.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>

#include "depend/jw_tcpconn.h"
#include "depend/jw_tcpserver.h"

class MyClass
{
public:
	MyClass();
	~MyClass();

	void Hello1() { printf("hello world1"); }
	void Hello2() { printf("hello world2"); }
	void Hello3() { printf("hello world3"); }
private:

};

MyClass::MyClass()
{
}

MyClass::~MyClass()
{
}

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
		MyClass *m = new MyClass;
		conn->OnConnected([&m](TcpConn* conn) {
			m->Hello1();
		});

		conn->OnDisconnected([&m](TcpConn* conn) {
			m->Hello2();
		});

		conn->OnRead([&m](TcpConn* conn, char* buf, int32_t len)->bool {
			m->Hello3();
			return true;
		});
	});
	server.Start("127.0.0.1", 8001);
	
	system("pause");
	server.Stop();
	system("pause");
	JwSocket::Clean();
	return 0;

}
