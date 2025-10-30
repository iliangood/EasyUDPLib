#if !defined UDP_SOCKET_H
#define UDP_SOCKET_H

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

using socket_t = SOCKET;

#define CLOSE_SOCKET close_socket
#define SOCKET_ERROR_CODE WSAGetLastError()

#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

using socket_t = int;

#define CLOSE_SOCKET close
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR_CODE errno
#define SOCKET_ERROR (-1)

#endif


#include <cstring>
#include <stdexcept>
#include <expected>

#include <message.h>
#include <ipaddress.h>


enum class UDPError {
    SUCCESS = 0,                    // 0 — успех (обе ОС)

    PERMISSION_DENIED,              // Linux: EACCES       | Windows: WSAEACCES
    MESSAGE_TOO_LARGE,              // Linux: EMSGSIZE     | Windows: WSAEMSGSIZE
    NO_BUFFER_SPACE,                // Linux: ENOBUFS      | Windows: WSAENOBUFS
    WOULD_BLOCK,                    // Linux: EAGAIN/WOULDBLOCK | Windows: WSAEWOULDBLOCK
    INVALID_SOCKET_DESC,            // Linux: EBADF        | Windows: WSAENOTSOCK
    INVALID_ARGUMENT,               // Linux: EINVAL       | Windows: WSAEINVAL
    NETWORK_DOWN,                   // Linux: ENETDOWN     | Windows: WSAENETDOWN
    ADDRESS_NOT_AVAILABLE,          // Linux: EADDRNOTAVAIL| Windows: WSAEADDRNOTAVAIL
    OPERATION_NOT_SUPPORTED,        // Linux: EOPNOTSUPP   | Windows: WSAEOPNOTSUPP
    SOCKET_CLOSED,                  // Linux: EPIPE        | Windows: WSAESHUTDOWN
    INTERRUPTED,                    // Linux: EINTR        | Windows: WSAEINTR

    WSA_NOT_INITIALIZED,            // Windows only: WSANOTINITIALISED
    WSA_IN_PROGRESS,                // Windows only: WSAEINPROGRESS

    MEMORY_FAULT,                   // Linux only: EFAULT
    DEST_ADDRESS_REQUIRED,          // Linux only: EDESTADDRREQ

    UNKNOWN                         // Любой другой код
};

UDPError last_udp_error();

std::string udp_error_to_string(UDPError err);

struct ReceiveInfo
{
	size_t dataSize;
	std::optional<IPAddress> remoteIP;
};

inline bool recieved(ReceiveInfo rcInfo)
{
	return rcInfo.remoteIP.has_value();
}

inline constexpr ReceiveInfo RECEIVE_NONE(0, std::nullopt);


class UDPSocket
{
	socket_t sock_;
	uint16_t port_;
	uint32_t intefaceIP_;

	std::optional<UDPError> bind(); 
public:

	UDPSocket();
	~UDPSocket();

	void reset();

	std::optional<UDPError> bind(uint16_t port); // port should be big-endian
	std::optional<UDPError> bindInteface(IPAddress ip);
	std::optional<UDPError> bindInteface(uint32_t ip);

	std::variant<size_t, UDPError> send_to(const uint8_t* data, size_t size, uint32_t ip); // ip should be big-endian
	std::variant<size_t, UDPError> send_to(const uint8_t* data, size_t size, IPAddress ip);

	std::variant<ReceiveInfo, UDPError> recieve(uint8_t* buf, size_t size);

};


#endif