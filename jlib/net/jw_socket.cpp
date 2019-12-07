#include "jw_socket.h"
#include <stdio.h>
#ifdef WIN32
#pragma comment(lib, "wsock32")
#endif

JwSocket::JwSocket()
{
	socket_fd_ = INVALID_SOCKET;
	local_addr_.sin_family = AF_INET;
	local_addr_.sin_addr.s_addr = 0;
	local_addr_.sin_port = 0;
	remote_addr_.sin_family = AF_INET;
	remote_addr_.sin_addr.s_addr = 0;
	remote_addr_.sin_port = 0;
}

JwSocket::~JwSocket()
{
}

int JwSocket::Init()
{
#ifdef WIN32
	/*
	http://msdn.microsoft.com/zh-cn/vstudio/ms741563(en-us,VS.85).aspx

	typedef struct WSAData { 
		WORD wVersion;								//winsock version
		WORD wHighVersion;							//The highest version of the Windows Sockets specification that the Ws2_32.dll can support
		char szDescription[WSADESCRIPTION_LEN+1]; 
		char szSystemStatus[WSASYSSTATUS_LEN+1]; 
		unsigned short iMaxSockets; 
		unsigned short iMaxUdpDg; 
		char FAR * lpVendorInfo; 
	}WSADATA, *LPWSADATA; 
	*/
	WSADATA wsaData;
	//#define MAKEWORD(a,b) ((WORD) (((BYTE) (a)) | ((WORD) ((BYTE) (b))) << 8)) 
	WORD version = MAKEWORD(2, 0);
	int ret = WSAStartup(version, &wsaData);//win sock start up
	if ( ret ) {
//		cerr << "Initilize winsock error !" << endl;
		return -1;
	}
#endif
	
	return 0;
}
//this is just for windows
int JwSocket::Clean()
{
#ifdef WIN32
		return (WSACleanup());
#endif
		return 0;
}

JwSocket::operator SOCKET ()
{
	return socket_fd_;
}

bool JwSocket::SetNoBlock(bool enable)
{
#ifdef WIN32
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the 
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled; 
	// If iMode != 0, non-blocking mode is enabled.
	u_long block = enable ? 1 : 0;
	int ret = ioctlsocket(socket_fd_, FIONBIO, &block);
	if (ret != NO_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
		return false;
	}
#else
	bool rc = false;
	int flags = fcntl(fd_, F_GETFL, 0);
	if (flags == -1) {
		printf("fcntl failed 111!\n");
		return false;
	}
	flags |= O_NONBLOCK;  // set nonblocking
	if (fcntl(fd_, F_SETFL, flags) == -1) {
		printf("fcntl failed 222!\n");
		return false;
	}
#endif // WIN32
	return true;
}

int JwSocket::GetRecvBufSize(int* size)
{
	socklen_t oplen = sizeof(*size);
	int ret = getsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, (char*)size, &oplen);
	if (ret == -1) {
#ifdef WIN32
		printf("GetRecvBufSize error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		return false;
	}
	return ret;
}

bool JwSocket::SetRecvBufSize(int size)
{
	int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size));
	if (ret == -1) {
#ifdef WIN32
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		
		return false;
	}
	return true;
}

int JwSocket::GetSendBufSize(int* size)
{
	socklen_t oplen = sizeof(*size);
	int ret = getsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, (char*)size, &oplen);
	if (ret == -1) {
#ifdef WIN32
		printf("GetRecvBufSize error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		
		return false;
	}
	return ret;
}

bool JwSocket::SetSendBufSize(int size)
{
	int ret = setsockopt(socket_fd_, SOL_SOCKET, SO_SNDBUF, (char*)&size, sizeof(size));
	if (ret == -1) {
#ifdef WIN32
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		
		return false;
	}
	return true;
}


bool JwSocket::SetNoDelay()
{
	int flag = 1;
	int ret = setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (ret == -1) {
#ifdef WIN32
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		
		return false;
	}
	return true;
}

bool JwSocket::Create(int af, int type, int protocol)
{
	socket_fd_ = socket(af, type, protocol);
	if ( socket_fd_ == INVALID_SOCKET ) {
#ifdef WIN32
		printf("socket failed with error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		return false;
	}
	return true;
}

bool JwSocket::Connect(const char* ip, unsigned short port)
{
	struct sockaddr_in svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.s_addr = inet_addr(ip);
 	svraddr.sin_port = htons(port);
	int ret = connect(socket_fd_, (struct sockaddr*)&svraddr, sizeof(svraddr));
	if ( ret == SOCKET_ERROR ) {
#ifdef WIN32
		printf("socket connect failed with error: %d\n", WSAGetLastError());
#else
		printf("GetRecvBufSize error: %d\n", errno);
#endif // WIN32
		return false;
	}
	//|| error == EINPROGRESS
	return true;
}



bool JwSocket::Bind(const char *ip, unsigned short port)
{
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	if (ip == NULL || ip[0] == '\0') {
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	if (inet_addr(ip) == INADDR_NONE){
		printf("address format error");
		return false;
	}

	serveraddr.sin_addr.s_addr = inet_addr(ip);
	
	serveraddr.sin_port = htons(port);

	int on = 1;
	int err = setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	if (err != 0) {
#ifdef WIN32
		return WSAGetLastError();
#else
		return errno;
#endif // WIN32
		
	}

	int ret = bind(socket_fd_, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if ( ret == SOCKET_ERROR ) {
		return false;
	}
	local_addr_ = serveraddr;
	return true;
}

bool JwSocket::Listen(int backlog)
{
	int ret = listen(socket_fd_, backlog);
	if ( ret == SOCKET_ERROR ) {
		return false;
	}
	return true;
}

bool JwSocket::Accept(JwSocket& socket)
{
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(cliaddr);
	SOCKET s = accept(socket_fd_, (struct sockaddr*)&cliaddr, &addrlen);
	if ( s == SOCKET_ERROR ) {
		return false;
	}
	
	socket = s;
	socket.setRemoteAddr(&cliaddr);
	return true;
}

int JwSocket::Send(const char* buf, int len, int flags)
{
	int bytes;
	int count = 0;
	while (count < len) {
		bytes = send(socket_fd_, buf + count, len - count, flags);
		if ( bytes == -1 || bytes == 0 )
			return -1;
		count += bytes;
	} 
	return count;
}

int JwSocket::Recv(char* buf, int len, int flags)
{
	return (recv(socket_fd_, buf, len, flags));
}

void JwSocket::Close()
{
#ifdef WIN32
	closesocket(socket_fd_);
#else
	close(socket_fd_);
#endif
}



bool JwSocket::DNSParse(const char* domain, char* ip)
{
	struct hostent* p;
	if ( (p = gethostbyname(domain)) == NULL )
		return false;
		
	sprintf(ip, 
		"%u.%u.%u.%u",
		(unsigned char)p->h_addr_list[0][0], 
		(unsigned char)p->h_addr_list[0][1], 
		(unsigned char)p->h_addr_list[0][2], 
		(unsigned char)p->h_addr_list[0][3]);
	
	return true;
}

#ifdef _MSC_VER
#else
bool JwSocket::SetKeepAlive(bool enabled)
{
	int value = enabled ? 1 : 0;
	return (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, (const void *)(&value), sizeof(value)) == 0);
}

/*
There's no direct relationship between those two options, they are just for different purposes.
TCP_NODELAY is intended to disable/enable segment buffering so data can be sent out to peer as quickly as possible
, so this is typically used to improve network utilisation. TCP_QUICKACK is used to send out acknowledgements
as early as possible than delayed under some protocol level exchanging, and it's not stable/permanent
, subsequent TCP transactions (which may happen under the hood) can disregard this option depending on actual protocol level
processing or any actual disagreements between user setting and stack behaviour.
NOTE TCP_NODELAY is portable while TCP_QUICKACK is not (only works under Linux 2.4.4+).
*/
bool TcpSession::SetTcpNoDelay(bool enabled)
{
	int value = enabled ? 1 : 0;
	return (setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, (const void *)(&value), sizeof(value)) == 0);
}
/*
TCP_QUICKACK Enable quickack mode if set or disable quickack mode if cleared
. In quickack mode, acks are sent immediately, rather than delayed if needed
in accordance to normal TCP operation. This flag is not permanent, it only
enables a switch to or from quickack mode. Subsequent operation of the TCP
protocol will once again enter/leave quickack mode depending on internal
protocol processing and factors such as delayed ack timeouts occurring and
data transfer. This option should not be used in code intended to be portable
*/
bool JwSocket::SetTcpQuickAck(bool enabled)
{
	int value = enabled ? 1 : 0;
	return (setsockopt(socket_fd_, IPPROTO_TCP, TCP_QUICKACK, (const void *)(&value), sizeof(value)) == 0);
}

bool JwSocket::SetSoLinger(bool do_linger, int seconds)
{
	struct linger linger_time;
	linger_time.l_onoff = do_linger ? 1 : 0;
	linger_time.l_linger = seconds;
	return (setsockopt(socket_fd_, SOL_SOCKET, SO_LINGER, (const void *)(&linger_time), sizeof(linger_time)) == 0);
}

bool JwSocket::SetTimeOption(int option, int milliseconds)
{
	struct timeval timeout;
	timeout.tv_sec = (int)(milliseconds / 1000);
	timeout.tv_usec = (milliseconds % 1000) * 1000000;
	return (setsockopt(socket_fd_, SOL_SOCKET, option, (const void *)(&timeout), sizeof(timeout)) == 0);
}


#endif // _MSC_VER