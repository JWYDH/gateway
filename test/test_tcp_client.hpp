#pragma once
#include "jw.h"

int test_tcp_client()
{
	jw::TcpClient client;

	client.OnRead([](jw::TcpClient *conn, jw::RingBuf &buf) {
		int64_t r = buf.size();
		if (r > 0)
		{
			char *tmp_buf = (char*)malloc(r+1);
			r = buf.read(tmp_buf, r);
			tmp_buf[r+1] = '\0';
			JW_LOG(jw::LL_INFO,"recv: %s", tmp_buf);
			conn->DoWrite(tmp_buf, r);
		}
	});

	client.Start("10.10.14.48", 8000);
	getchar();
	return 0;
}