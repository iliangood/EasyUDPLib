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

UDPError last_udp_error() {
    int err = SOCKET_ERROR_CODE;

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
	errno = 0;
#endif
}

std::string udp_error_string(UDPError err) {
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

class UDPSocket
{
	socket_t sock_;
public:

	UDPSocket();
	~UDPSocket();
	
	int send_to(uint8_t* data, size_t size, uint32_t ip, uint16_t port); // ip and port should be big-endian
	int send_to(uint8_t* data, size_t size, IPAddress ip, uint16_t port); // port should be Host-endian

};


#endif