#pragma once
#include "jw.h"

int test_tcp_server()
{
	jw::TcpServer server;
	server.OnRead([&server](jw::TcpConn* conn, jw::RingBuf &buf) {
		int64_t r = buf.size();
		if (r > 0)
		{
			char *tmp_buf = (char*)malloc(r+1);
			r = buf.read(tmp_buf, r);
			tmp_buf[r+1] = '\0';
			JW_LOG(jw::LL_INFO,"recv: %s", tmp_buf);
			server.DoWrite(conn, tmp_buf, r);
		}
	});
	server.Start("0.0.0.0", 8000);

	getchar();

	server.Stop();
	return 0;
}