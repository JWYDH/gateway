#include "jw_socket.h"
#include "jw_log.h"

namespace jw
{

bool DNS_parse(const char* domain, char* ip)
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

socket_t create_socket(void)
{
	socket_t sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == INVALID_SOCKET)
	{
		JW_LOG(LL_ERROR, "socket_api::create_socket, err=%d", get_lasterror());
	}

	return sockfd;
}

void close_socket(socket_t s)
{
	if (SOCKET_ERROR == ::close(s))
	{
		JW_LOG(LL_ERROR, "socket_api::close_socket, err=%d", get_lasterror());
	}
}

bool set_recv_buf_size(socket_t s, int size);
{
	if (SOCKET_ERROR == ::setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&size, sizeof(size)))
	{
		JW_LOG(LL_ERROR, "set_recv_buf_size, err=%d", get_lasterror());
		return false;
	}
	return true;
}


bool set_send_buf_size(socket_t s, int size);
{
	if (SOCKET_ERROR == ::setsockopt(s, SO_SNDBUF, SO_RCVBUF, (char*)&size, sizeof(size)))
	{
		JW_LOG(LL_ERROR, "set_send_buf_size, err=%d", get_lasterror());
		return false;
	}
	return true;
}


bool set_nonblock(socket_t s, bool enable)
{

	int32_t flags = ::fcntl(s, F_GETFL, 0);
	if (SOCKET_ERROR == flags ||
		SOCKET_ERROR == ::fcntl(s, F_SETFL, enable ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK)))
	{
		JW_LOG(LL_ERROR, "socket_api::set_nonblock, err=%d", get_lasterror());
		return false;
	}

	return true;
}

bool set_close_onexec(socket_t s, bool enable)
{
	// close-on-exec
	int32_t flags = ::fcntl(s, F_GETFD, 0);
	if (SOCKET_ERROR == flags ||
		SOCKET_ERROR == ::fcntl(s, F_SETFD, enable ? (flags | FD_CLOEXEC) : (flags & ~FD_CLOEXEC)))
	{
		JW_LOG(LL_ERROR, "socket_api::set_close_onexec, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool bind(socket_t s, const struct sockaddr_in &addr)
{
	if (SOCKET_ERROR == ::bind(s, (const sockaddr *)(&addr), static_cast<socklen_t>(sizeof addr)))
	{
		JW_LOG(LL_ERROR, "socket_api::bind, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool listen(socket_t s)
{
	if (SOCKET_ERROR == ::listen(s, SOMAXCONN))
	{
		JW_LOG(LL_ERROR, "socket_api::listen, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool connect(socket_t s, const struct sockaddr_in &addr)
{

	int ret = connect(socket_fd_, (struct sockaddr*)&svraddr, sizeof(svraddr));
	if (::connect(s, (const sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		int lasterr = get_lasterror();

		//non-block socket
		if (lasterr == EINPROGRESS ||
			lasterr == EINTR ||
			lasterr == EISCONN)
			return true;

		JW_LOG(LL_ERROR, "socket_api::connect, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool inet_pton(const char *ip, struct in_addr &a)
{
	if (::inet_pton(AF_INET, ip, &a) != 1)
	{
		JW_LOG(LL_ERROR, "socket_api::inet_pton, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool inet_ntop(const struct in_addr &a, char *dst, socklen_t size)
{
	if (::inet_ntop(AF_INET, &a, dst, size) == 0)
	{
		JW_LOG(LL_ERROR, "socket_api::inet_ntop, err=%d", get_lasterror());
		return false;
	}
	return true;
}

ssize_t write(socket_t s, const char *buf, size_t len)
{
	ssize_t _len = (ssize_t)::write(s, buf, len);
	return _len;
}

ssize_t read(socket_t s, void *buf, size_t len)
{
	ssize_t _len = (ssize_t)::read(s, buf, len);
	return _len;
}

bool shutdown(socket_t s)
{
	if (SOCKET_ERROR == ::shutdown(s, SHUT_RDWR))
	{
		JW_LOG(LL_ERROR, "socket_api::shutdown, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool setsockopt(socket_t s, int level, int optname, const void *optval, size_t optlen)
{
	if (SOCKET_ERROR == ::setsockopt(s, level, optname, (const char *)optval, (socklen_t)optlen))
	{
		JW_LOG(LL_ERROR, "socket_api::setsockopt, level=%d, optname=%d, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool set_reuse_addr(socket_t s, bool on)
{
	int optval = on ? 1 : 0;
	return setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

bool set_reuse_port(socket_t s, bool on)
{
	int optval = on ? 1 : 0;
	return setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
}

bool set_keep_alive(socket_t s, bool on)
{
	int optval = on ? 1 : 0;
	return setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

bool set_nodelay(socket_t s, bool on)
{
	int optval = on ? 1 : 0;
	return setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}

bool set_linger(socket_t s, bool on, uint16_t linger_time)
{
	struct linger linger_;

	linger_.l_onoff = on; // && (linger_time > 0)) ? 1 : 0;
	linger_.l_linger = on ? linger_time : 0;

	return setsockopt(s, SOL_SOCKET, SO_LINGER, &linger_, sizeof(linger_));
}

int get_socket_error(socket_t sockfd)
{
	int optval;
	socklen_t optlen = sizeof(optval);

	if (SOCKET_ERROR == ::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *)&optval, &optlen))
	{
		return errno;
	}
	else
	{
		return optval;
	}
}

socket_t accept(socket_t s, struct sockaddr_in *addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof(sockaddr_in));
	socket_t connfd = ::accept(s, (struct sockaddr *)addr, addr ? (&addrlen) : 0);

	if (connfd == INVALID_SOCKET)
	{
		JW_LOG(LL_ERROR, "socket_api::accept, err=%d", get_lasterror());
	}
	return connfd;
}

bool getsockname(socket_t s, struct sockaddr_in &addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
	memset(&addr, 0, sizeof addr);

	if (SOCKET_ERROR == ::getsockname(s, (struct sockaddr *)(&addr), &addrlen))
	{
		JW_LOG(LL_ERROR, "socket_api::getsockname, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool getpeername(socket_t s, struct sockaddr_in &addr)
{
	socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
	memset(&addr, 0, sizeof addr);

	if (SOCKET_ERROR == ::getpeername(s, (struct sockaddr *)(&addr), &addrlen))
	{
		JW_LOG(LL_ERROR, "socket_api::getpeername, err=%d", get_lasterror());
		return false;
	}
	return true;
}

bool resolve_hostname(const char *hostname, struct sockaddr_in &addr)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo *result = nullptr;
	if (getaddrinfo(hostname, nullptr, &hints, &result) != 0)
	{
		JW_LOG(LL_ERROR, "socket_api::resolve %s failed, errno=%d", hostname, get_lasterror());
		return false;
	}

	bool find_ipv4_address = false;
	for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		if (ptr->ai_family != AF_INET)
			continue;

		//AF_INET (IPv4)
		addr.sin_addr = ((struct sockaddr_in *)ptr->ai_addr)->sin_addr;
		find_ipv4_address = true;
		break;
	}

	if (!find_ipv4_address)
	{
		JW_LOG(LL_ERROR, "socket_api::resolve %s failed, IPV4 address not found", hostname);
	}

	freeaddrinfo(result);
	return find_ipv4_address;
}

uint16_t ntoh_16(uint16_t x)
{
	return ntohs(x);
}

uint32_t ntoh_32(uint32_t x)
{
	return ntohl(x);
}

int get_lasterror(void)
{
	return errno;
}

bool is_lasterror_WOULDBLOCK(void)
{
	return errno == EWOULDBLOCK;
}

} // namespace jw