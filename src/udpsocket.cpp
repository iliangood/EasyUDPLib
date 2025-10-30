#include <udpsocket.h>

#include <iostream>

UDPSocket::UDPSocket()
{
	reset();
}

UDPSocket::UDPSocket(uint32_t port)
{
	bind(port);
}

UDPSocket::UDPSocket(uint32_t port, IPAddress ip)
{
	bind(port);
	bindInteface(ip);
}

UDPSocket::~UDPSocket()
{
	if(sock_ != INVALID_SOCKET)
		CLOSE_SOCKET(sock_);
#if defined _WIN32
	WSACleanup();
#endif
}

void UDPSocket::reset()
{
    if (sock_ != INVALID_SOCKET) 
	{
        CLOSE_SOCKET(sock_);
        sock_ = INVALID_SOCKET;
    }

#if defined(_WIN32)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
#endif

    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ == INVALID_SOCKET)
        throw std::runtime_error("socket() failed");

    int enable = 1;
    setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

#if defined(_WIN32)
    u_long mode = 1;
    ioctlsocket(sock_, FIONBIO, &mode);
#else
    long flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
#endif
}

std::optional<UDPError> UDPSocket::bind()
{
	reset();

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = intefaceIP_;
	addr.sin_port = port_;
	int rc = ::bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(rc == 0)
		return std::nullopt;
	return last_udp_error();
}

std::optional<UDPError> UDPSocket::bind(uint16_t port)
{
	port_ = port;
	return bind();
}

std::optional<UDPError> UDPSocket::bindInteface(uint32_t ip)
{
	intefaceIP_ = ip;
	return bind();
}

std::optional<UDPError> UDPSocket::bindInteface(IPAddress ip)
{
	return bindInteface(ip.toNet());
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
		updateIPs();
		if(reinterpret_cast<sockaddr*>(msg.msg_name)->sa_family == AF_INET)
		{
			IPAddress ip = IPAddress::fromNet(reinterpret_cast<sockaddr_in*>(msg.msg_name)->sin_addr.s_addr);
			if(std::find(IPs.begin(), IPs.end(), ip) == IPs.end())
				return ReceiveInfo(rc, ip);
			std::cout<<"reject by ip" <<std::endl;
			return RECEIVE_NONE;
		}
		return ReceiveInfo(rc, IP_ANY);
	}

	UDPError rcE = last_udp_error();
	if(rcE == UDPError::WOULD_BLOCK)
		return RECEIVE_NONE;
	return rcE;
}

uint16_t UDPSocket::getBindPort()
{
	return port_;
}
uint32_t UDPSocket::getBindInterface()
{
	return intefaceIP_;
}

std::vector<IPAddress> intefacesIPs()
{
	ifaddrs* ifaddr;
	int family;

	std::vector<IPAddress> IPs;

	if(getifaddrs(&ifaddr) != 0)
		return {};
	for (struct ifaddrs *ifa = ifaddr; ifa != NULL;
			ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET) 
		{
			sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
			IPs.push_back(IPAddress::fromNet(addr->sin_addr.s_addr));
		}

	}
	freeifaddrs(ifaddr);
	return IPs;
}

void updateIPs()
{
	auto now = std::chrono::steady_clock::now();

    if (last_update == std::chrono::steady_clock::time_point{}) {
        last_update = now; 

        return;
    }

    auto elapsed = now - last_update;

    if (elapsed > std::chrono::seconds(10)) 
	{
		IPs = intefacesIPs();
        last_update = now; 
    }
}

UDPError last_udp_error() {
    int err = SOCKET_ERROR_CODE;
	errno = 0;
#ifdef _WIN32
    switch (err) {
        case 0:                     return UDPError::SUCCESS;
        case WSAEACCES:             return UDPError::PERMISSION_DENIED;
        case WSAEMSGSIZE:           return UDPError::MESSAGE_TOO_LARGE;
        case WSAENOBUFS:            return UDPError::NO_BUFFER_SPACE;
        case WSAEWOULDBLOCK:        return UDPError::WOULD_BLOCK;
        case WSAENOTSOCK:           return UDPError::INVALID_SOCKET_DESC;
        case WSAEINVAL:             return UDPError::INVALID_ARGUMENT;
        case WSAENETDOWN:           return UDPError::NETWORK_DOWN;
        case WSAEADDRNOTAVAIL:      return UDPError::ADDRESS_NOT_AVAILABLE;
        case WSAEOPNOTSUPP:         return UDPError::OPERATION_NOT_SUPPORTED;
        case WSAESHUTDOWN:          return UDPError::SOCKET_CLOSED;
        case WSAEINTR:              return UDPError::INTERRUPTED;
        case WSANOTINITIALISED:     return UDPError::WSA_NOT_INITIALIZED;
        case WSAEINPROGRESS:        return UDPError::WSA_IN_PROGRESS;
        default:                    return UDPError::UNKNOWN;
    }
#else
    switch (err) {
        case 0:                     return UDPError::SUCCESS;
        case EACCES:                return UDPError::PERMISSION_DENIED;
        case EMSGSIZE:              return UDPError::MESSAGE_TOO_LARGE;
        case ENOBUFS:               return UDPError::NO_BUFFER_SPACE;
        case EWOULDBLOCK:           return UDPError::WOULD_BLOCK;
        case EBADF:                 return UDPError::INVALID_SOCKET_DESC;
        case EINVAL:                return UDPError::INVALID_ARGUMENT;
        case ENETDOWN:              return UDPError::NETWORK_DOWN;
        case EADDRNOTAVAIL:         return UDPError::ADDRESS_NOT_AVAILABLE;
        case EOPNOTSUPP:            return UDPError::OPERATION_NOT_SUPPORTED;
        case EPIPE:                 return UDPError::SOCKET_CLOSED;
        case EINTR:                 return UDPError::INTERRUPTED;
        case EFAULT:                return UDPError::MEMORY_FAULT;
        case EDESTADDRREQ:          return UDPError::DEST_ADDRESS_REQUIRED;
        default:                    return UDPError::UNKNOWN;
    }
#endif
}

std::string udp_error_to_string(UDPError err) {
    switch (err) {
        case UDPError::SUCCESS:               return "Success";
        case UDPError::PERMISSION_DENIED:     return "Permission denied (EACCES/WSAEACCES)";
        case UDPError::MESSAGE_TOO_LARGE:     return "Message too large (EMSGSIZE/WSAEMSGSIZE)";
        case UDPError::NO_BUFFER_SPACE:       return "No buffer space (ENOBUFS/WSAENOBUFS)";
        case UDPError::WOULD_BLOCK:           return "Would block (EAGAIN/WSAEWOULDBLOCK)";
        case UDPError::INVALID_SOCKET_DESC:   return "Invalid socket descriptor (EBADF/WSAENOTSOCK)";
        case UDPError::INVALID_ARGUMENT:      return "Invalid argument (EINVAL/WSAEINVAL)";
        case UDPError::NETWORK_DOWN:          return "Network down (ENETDOWN/WSAENETDOWN)";
        case UDPError::ADDRESS_NOT_AVAILABLE: return "Address not available (EADDRNOTAVAIL/WSAEADDRNOTAVAIL)";
        case UDPError::OPERATION_NOT_SUPPORTED:return "Operation not supported (EOPNOTSUPP/WSAEOPNOTSUPP)";
        case UDPError::SOCKET_CLOSED:         return "Socket closed (EPIPE/WSAESHUTDOWN)";
        case UDPError::INTERRUPTED:           return "Interrupted (EINTR/WSAEINTR)";
        case UDPError::WSA_NOT_INITIALIZED:   return "Winsock not initialized (WSANOTINITIALISED)";
        case UDPError::WSA_IN_PROGRESS:       return "Operation in progress (WSAEINPROGRESS)";
        case UDPError::MEMORY_FAULT:          return "Memory fault (EFAULT)";
        case UDPError::DEST_ADDRESS_REQUIRED: return "Destination address required (EDESTADDRREQ)";
        case UDPError::UNKNOWN:               return "Unknown error";
        default:                              return "Invalid error code";
    }
}

