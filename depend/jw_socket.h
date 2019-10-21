#pragma once
#include "../stdafx.h"

class JwSocket {

public:
	JwSocket();
	~JwSocket();

	operator SOCKET ();
	JwSocket& operator = (SOCKET s) {
		socket_fd_ = s;
		return (*this);
	}
	inline void getLoaclAddr(sockaddr_in* addr_in)
	{
		*addr_in = local_addr_;
	}
	//��ȡԶ�̵ĵ�ַ�Ͷ˿ڵ�sockaddr_in�ṹ
	inline void getRemoteAddr(sockaddr_in* addr_in)
	{
		*addr_in = remote_addr_;
	}
	
	inline void setLoaclAddr(const sockaddr_in* addr_in)
	{
		local_addr_ = *addr_in;
	}
	//����Զ�̵�ַ��Ϣ
	inline void setRemoteAddr(const sockaddr_in* addr_in)
	{
		remote_addr_ = *addr_in;
	}
	//============================================
	bool SetNoBlock(bool enable = true);
	int GetRecvBufSize(int* size);
	bool SetRecvBufSize(int size);
	int GetSendBufSize(int* size);
	bool SetSendBufSize(int size);

	bool SetNoDelay();

	bool Create(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);

	bool Connect(const char* ip, unsigned short port);

	bool Bind(const char *ip, unsigned short port);

	bool Listen(int backlog = 5); 


	bool Accept(JwSocket& socket);

	int Send(const char* buf, int len, int flags = 0);


	int Recv(char* buf, int len, int flags = 0);
	
	void Shutdown(int s);

	void Close();


	
	static int Init();	

	static int Clean();

	static bool DNSParse(const char* domain, char* ip);
protected:
	SOCKET socket_fd_;
	sockaddr_in	local_addr_;	//�󶨵ı��ص�ַ
	sockaddr_in	remote_addr_;	//Զ�̵�ַ
};

