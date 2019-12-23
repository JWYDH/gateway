//#include <iostream>
//
//#include "jw_mysql_client.h"
//#include "tcp_conn.h"
//#include "tcp_server.h"
//#include "jw_events.h"
//
////using namespace webapp;
//
//int main() {
//
//	// 3��ʹ�ö�ȡ���Ĳ�����ѯ���ݿ⣨MysqlClient��
//
//	Events events;
//	TcpServer server(&events);
//	server.Start("0.0.0.0", 8080);
//
//	server.OnRead([](TcpConn& conn) {
//		int64_t r = conn.read_buff_.Length();
//		if (r > 0) {
//			char buf[65536] = {};
//			r = conn.read_buff_.ReadBuf(buf, r);
//			buf[r] = '\0';
//			cout << "recv: " << buf << endl;
//
//			char sql[256] = { 0 };
//			sprintf(sql, "SELECT user,host FROM user", "root");
//			
//
//			MysqlClient mysqlclient;
//			MysqlData mysqldata;
//			mysqlclient.connect("192.168.1.123", "root", "MySql5.7", "mysql");
//			if (mysqlclient.is_connected()) {
//				if (mysqlclient.query(sql, mysqldata)) {
//					for (size_t i = 0; i < mysqldata.cols(); i++)
//					{
//						cout << mysqldata.field_name(i);
//						cout << "|";
//					}
//					cout << endl;
//					for (size_t i = 0; i < mysqldata.rows(); i++)
//					{
//						for (size_t j = 0; j < mysqldata.cols(); j++)
//						{
//							cout << mysqldata(i, j);
//							cout << "|";
//						}
//						cout << endl;
//					}
//				}
//				else {
//					cout << mysqlclient.error() << endl;
//				}
//			}
//			conn.DoWrite(buf, r);
//
//
//		}
//		});
//	events.Loop();
//	return 0;
//}
//
// mysqlclient
#include "tcp_conn.h"
#include "tcp_server.h"
#include "jw_events.h"

int main()
{
	Events events;
	TcpServer server(&events);
	server.Start("10.10.14.192", 8080);

	server.OnRead([](TcpConn& conn) {
		int64_t r = conn.read_buff_.Length();
		if (r > 0) {
			char buf[65536] = {};
			r = conn.read_buff_.ReadBuf(buf, r);
			buf[r] = '\0';
			printf("recv: %s", buf);
			conn.DoWrite(buf, r);
		}
	});
	events.Loop();
	return 0;
}