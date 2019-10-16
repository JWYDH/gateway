

#include <WinSock2.h>  
#include <Windows.h>  
#include <MSWSock.h>  
#include <stdio.h>  
#include <map>  
using namespace std;


#pragma comment(lib,"Ws2_32.lib")  
#pragma comment(lib,"Mswsock.lib")  

//FD_SETSIZE是在winsocket2.h头文件里定义的，这里windows默认最大为64
//在包含winsocket2.h头文件前使用宏定义可以修改这个值
int main()
{
	WSAData wsaData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("初始化失败!%d\n", WSAGetLastError());
		Sleep(5000);
		return -1;
	}

	USHORT nport = 8888;
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	u_long ul = 1;
	ioctlsocket(sListen, FIONBIO, &ul);

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(nport);
	sin.sin_addr.S_un.S_addr = ADDR_ANY;


	if (SOCKET_ERROR == bind(sListen, (sockaddr*)&sin, sizeof(sin)))
	{
		printf("bind failed!%d\n", WSAGetLastError());
		Sleep(5000);
		return -1;
	}


	listen(sListen, 5);


	//1)初始化一个套接字集合fdSocket，并将监听套接字放入  
	fd_set socketSet;
	FD_ZERO(&socketSet);
	FD_SET(sListen, &socketSet);

	TIMEVAL time = { 1,0 };
	char buf[4096];


	fd_set    readSet;
	FD_ZERO(&readSet);

	fd_set    writeSet;
	FD_ZERO(&readSet);


	while (true)
	{
		//2）将fdSocket的一个拷贝fdRead传给select函数  
		readSet = socketSet;
		writeSet = socketSet;

		//同时检查套接字的可读可写性。
		int   nRetAll = select(0, &readSet, &writeSet, NULL, NULL/*&time*/);//若不设置超时则select为阻塞  
		if (nRetAll >0)   //-1
		{
			//是否存在客户端的连接请求。  
			if (FD_ISSET(sListen, &readSet))//在readset中会返回已经调用过listen的套接字。  
			{

				if (socketSet.fd_count < FD_SETSIZE)
				{
					sockaddr_in addrRemote;
					int nAddrLen = sizeof(addrRemote);
					SOCKET sClient = accept(sListen, (sockaddr*)&addrRemote, &nAddrLen);
					if (sClient != INVALID_SOCKET)
					{
						FD_SET(sClient, &socketSet);
						printf("\n接收到连接：(%s)", inet_ntoa(addrRemote.sin_addr));
					}
				}
				else
				{
					printf("连接数量已达上限！\n");
					continue;
				}
			}


			for (int i = 0; i<socketSet.fd_count; i++)
			{
				if (FD_ISSET(socketSet.fd_array[i], &readSet))
				{
					//调用recv，接收数据。 
					int nRecv = recv(socketSet.fd_array[i], buf, 4096, 0);
					if (nRecv > 0)
					{
						buf[nRecv] = 0;
						printf("\nrecv  %d :  %s", socketSet.fd_array[i], buf);
					}
				}

				if (FD_ISSET(socketSet.fd_array[i], &writeSet))
				{

					//调用send，发送数据。
					char buf[] = "hello!";
					int nRet = send(socketSet.fd_array[i], buf, strlen(buf) + 1, 0);
					if (nRet <= 0)
					{
						if (GetLastError() == WSAEWOULDBLOCK)
						{
							//do nothing  
						}
						else
						{
							closesocket(socketSet.fd_array[i]);
							FD_CLR(socketSet.fd_array[i], &socketSet);
						}
					}
					else
					{
						printf("\nsend  hello!");
					}
				}
			}

		}
		else if (nRetAll == 0)
		{
			printf("time out!\n");
		}
		else
		{
			printf("select error!%d\n", WSAGetLastError());
			Sleep(5000);
			break;
		}

		Sleep(1000);

	}
	closesocket(sListen);
	WSACleanup();
}
