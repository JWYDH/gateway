// gateway.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>

#include "depend/jw_tcpconn.h"
#include "depend/jw_tcpserver.h"
#include "depend/3rd//http_parser.h"
#include "depend/3rd/HttpFormat.h"
#include "depend/3rd/HttpParser.h"
#include "depend/3rd/WebSocketFormat.h"


class MyClass
{
public:
	MyClass();
	~MyClass();

	void OnConnected() {}
	void OnDisconnected() {}
	int32_t OnRead(TcpConn* conn, const char* buf, int32_t len) {
		conn->DoWrite(buf, len);
		if (!upgrade_){
			int32_t retlen = len;
			auto httpParser = std::make_shared<HTTPParser>(HTTP_REQUEST);
			if (!httpParser->isCompleted())
			{
				retlen = httpParser->tryParse(buf, len);
				if (!httpParser->isCompleted())
				{
					return retlen;
				}
			}
			if (httpParser->isWebSocket())
			{
				if (httpParser->hasKey("Sec-WebSocket-Key"))
				{
					auto response = WebSocketFormat::wsHandshake(httpParser->getValue("Sec-WebSocket-Key"));
					conn->DoWrite(response.c_str(), response.size());
				}
			}
			else
			{
				if (httpParser->isKeepAlive())
				{
					httpParser->clearParse();
				}
			}
			return retlen;
		}
		else {
		}
		return 0;
	}

private:
	bool upgrade_;
};

MyClass::MyClass()
:upgrade_(false){
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
		//conn->OnConnected([&m](TcpConn* conn) {
		//	m->OnConnected();
		//});

		//conn->OnDisconnected([&m](TcpConn* conn) {
		//	m->OnDisconnected();
		//});
		conn->OnRead([m](TcpConn* conn, char* buf, int32_t len)->int32_t {
			int32_t r = m->OnRead(conn, buf, len);
			return r;
		});
	});
	//server.Start("127.0.0.1", 8001);
	server.Start("192.168.1.7", 8001);
	
	system("pause");
	server.Stop();
	
	JwSocket::Clean();
	return 0;

}


