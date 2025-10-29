#include <udpsocket.h>

UDPSocket::UDPSocket()
{
#if defined _WIN32
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		throw std::runtime("WSAStartup failed");
#endif
sock_ = socket(AF_INET, SOCK_DGRAM, 0);
if(sock_ == INVALID_SOCKET)
	throw std::runtime_error("void UDPSocket::UDPSocket() socket() failed");
}

UDPSocket::~UDPSocket()
{
	if(sock_ != INVALID_SOCKET)
		CLOSE_SOCKET(sock_);
#if defined _WIN32
	WSACleanup();
#endif
}

int UDPSocket::send_to(uint8_t* data, size_t size, uint32_t ip, uint16_t port) // ip and port should be big-endian
{
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip;
	addr.sin_port = port;

	return sendto(sock_, data, size, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

int UDPSocket::send_to(uint8_t* data, size_t size, IPAddress ip, uint16_t port) // port should be Host-endian
{
	return send_to(data, size, ip.toNet(), hton(port));
}