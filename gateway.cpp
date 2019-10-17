// gateway.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>

#include "depend/jw_tcpconn.h"
#include "depend/jw_tcpserver.h"
#include "depend/3rd//http_parser.h"
#include "depend/3rd/WebSocketFormat.h"


class MyClass
{
public:
	MyClass();
	~MyClass();

	void OnConnected() {}
	void OnDisconnected() {}
	void OnRead(const char* buf, int32_t len) {
		http_parser_init(&parser_, HTTP_BOTH);
		size_t nparsed = http_parser_execute(&parser_, &settings_, buf, len);
		//size_t retlen = 0;
		//if (websocket_)
		//{
		//	WebSocketFormat::wsHandshake()
		//}
		//if (->isWebSocket())
		//{
		//	retlen = HttpService::ProcessWebSocket(buffer, len, httpParser, httpSession);
		//}
		//else
		//{
		//	retlen = HttpService::ProcessHttp(buffer, len, httpParser, httpSession);
		//}

		//return retlen;
	}
private:
	static int                              sChunkHeader(http_parser* hp){ return 0; }
	static int                              sChunkComplete(http_parser* hp){ return 0; }
	static int                              sMessageBegin(http_parser* hp){ 
		return 0;
	}
	static int                              sMessageEnd(http_parser* hp){ return 0; }
	static int                              sHeadComplete(http_parser* hp){ return 0; }
	static int                              sUrlHandle(http_parser* hp, const char *url, size_t length){ 
		return 0;
	}
	static int                              sHeadValue(http_parser* hp, const char *at, size_t length){ 
		return 0;
	}
	static int                              sHeadField(http_parser* hp, const char *at, size_t length){ 
		return 0;
	}
	static int                              sStatusHandle(http_parser* hp, const char *at, size_t length){ 
		return 0;
	}
	static int                              sBodyHandle(http_parser* hp, const char *at, size_t length){ 
		return 0;
	}
	//HTTPParser httpParser;
	bool websocket_ = false;
	http_parser                             parser_;
	http_parser_settings                    settings_;
};

MyClass::MyClass()
{
	settings_.on_status = sStatusHandle;
	settings_.on_body = sBodyHandle;
	settings_.on_url = sUrlHandle;
	settings_.on_header_field = sHeadField;
	settings_.on_header_value = sHeadValue;
	settings_.on_headers_complete = sHeadComplete;
	settings_.on_message_begin = sMessageBegin;
	settings_.on_message_complete = sMessageEnd;
	settings_.on_chunk_header = sChunkHeader;
	settings_.on_chunk_complete = sChunkComplete;
	parser_.data = this;
	//http_parser_init(&parser_, HTTP_BOTH);
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
		conn->OnRead([m](TcpConn* conn, char* buf, int32_t len)->bool {
			m->OnRead(buf, len);
			return true;
		});
	});
	server.Start("127.0.0.1", 8001);
	
	system("pause");
	server.Stop();
	
	JwSocket::Clean();
	return 0;

}
