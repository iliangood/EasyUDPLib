#include <udpsocket.h>

UDPSocket::UDPSocket()
{
#if defined _WIN32
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		throw std::runtime_error("WSAStartup failed");
#endif
	sock_ = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_ == INVALID_SOCKET)
		throw std::runtime_error("void UDPSocket::UDPSocket() socket() failed");
	int enable = 1;
	setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

#if defined _WIN32
	u_long mode = 1;
	ioctlsocket(sock_, FIONBIO, &mode);
#else
	long flags = fcntl(sock_, F_GETFL, 0);
	fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
#endif
}

UDPSocket::~UDPSocket()
{
	if(sock_ != INVALID_SOCKET)
		CLOSE_SOCKET(sock_);
#if defined _WIN32
	WSACleanup();
#endif
}

std::optional<UDPError> UDPSocket::bind(uint16_t port)
{
	port_ = port;
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = port;
	int rc = ::bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(rc == 0)
		return std::nullopt;
	return last_udp_error();
}

std::variant<size_t, UDPError> UDPSocket::send_to(const uint8_t* data, size_t size, uint32_t ip) // ip should be big-endian
{
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = port_;

	int rc = sendto(sock_, data, size, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(rc >= 0)
		return size;
	return last_udp_error();
}

std::variant<size_t, UDPError> UDPSocket::send_to(const uint8_t* data, size_t size, IPAddress ip)
{
	return send_to(data, size, ip.toNet());
}

std::variant<ReceiveInfo, UDPError> UDPSocket::recieve(uint8_t* buf, size_t size)
{
	sockaddr_in srcaddr{};
	iovec iov{};
	iov.iov_base = buf;
	iov.iov_len = size;

	msghdr msg{};
	msg.msg_name = &srcaddr;
	msg.msg_namelen = sizeof(srcaddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = nullptr;
	msg.msg_controllen = 0;

	ssize_t rc = recvmsg(sock_, &msg, 0);
	if(rc >= 0)
	{
		if(reinterpret_cast<sockaddr*>(msg.msg_name)->sa_family == AF_INET)		
			return ReceiveInfo(rc, IPAddress::fromNet(reinterpret_cast<sockaddr_in*>(msg.msg_name)->sin_addr.s_addr));
		return ReceiveInfo(rc, IP_ANY);
	}

	UDPError rcE = last_udp_error();
	if(rcE == UDPError::WOULD_BLOCK)
		return ReceiveInfo(0, std::nullopt);
	return rcE;
}