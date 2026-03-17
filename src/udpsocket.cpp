#include "udpsocket.h"

#include <iostream>
#include <cstring>      // memset
#include <stdexcept>
#include <optional>
#include <vector>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <ifaddrs.h>
    #include <errno.h>
#endif

#ifdef _WIN32
namespace {
    struct WinsockInitializer {
        WinsockInitializer() {
            WSADATA wsa;
            int result = WSAStartup(MAKEWORD(2, 2), &wsa);
            if (result != 0) {
                std::cerr << "WSAStartup failed with error: " << result << std::endl;
                // Можно throw, но лучше не, чтобы не ломать программу
            }
        }

        ~WinsockInitializer() {
            WSACleanup();
        }
    };

    static WinsockInitializer winsock_init;  // вызывается при загрузке DLL/программы
}
#endif

// ────────────────────────────────────────────────
//  Кроссплатформенные определения
// ────────────────────────────────────────────────

#ifdef _WIN32
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
    constexpr int SOCK_ERROR = SOCKET_ERROR;
    #define CLOSE_SOCK(s)    closesocket(s)
    #define LAST_ERROR()     WSAGetLastError()
    #define WOULD_BLOCK_ERR  WSAEWOULDBLOCK
#else
    using socket_t = int;
    constexpr socket_t INVALID_SOCK = -1;
    constexpr int SOCK_ERROR = -1;
    #define CLOSE_SOCK(s)    ::close(s)
    #define LAST_ERROR()     errno
    #define WOULD_BLOCK_ERR  EWOULDBLOCK
#endif

// ────────────────────────────────────────────────
//  UDPSocket реализация
// ────────────────────────────────────────────────

UDPSocket::UDPSocket(uint16_t port) : intefaceIP_(INADDR_ANY)
{
    std::optional<UDPError> rc = bind(port);
    if (rc.has_value())
    {
        throw std::runtime_error("UDPSocket::UDPSocket(uint16_t) bind failed: " + udp_error_to_string(rc.value()));
    }
}

UDPSocket::UDPSocket(uint16_t port, IPAddress ip)
{
    intefaceIP_ = ip.toNet();
    std::optional<UDPError> rc = bind(port);
    if (rc.has_value())
    {
        throw std::runtime_error("UDPSocket bind failed: " + udp_error_to_string(rc.value()));
    }
}

UDPSocket::~UDPSocket()
{
    if (sock_ != INVALID_SOCK)
        CLOSE_SOCK(sock_);

#ifdef _WIN32
    WSACleanup();
#endif
}

UDPSocket::UDPSocket(UDPSocket&& other) noexcept
{
    sock_ = other.sock_;
    other.sock_ = INVALID_SOCK;

    port_ = other.port_;
    other.port_ = 0;

    intefaceIP_ = other.intefaceIP_;
    other.intefaceIP_ = IPAddress(0,0,0,0).toNet();
}

UDPSocket& UDPSocket::operator=(UDPSocket&& other) noexcept
{
    if (this != &other)
    {
        if (sock_ != INVALID_SOCK)
        {
            CLOSE_SOCK(sock_);
        }

        sock_ = other.sock_;
        other.sock_ = INVALID_SOCK;

        port_ = other.port_;
        other.port_ = 0;

        intefaceIP_ = other.intefaceIP_;
        other.intefaceIP_ = IPAddress(0,0,0,0).toNet();
    }
    return *this;
}

void UDPSocket::reset()
{
    if (sock_ != INVALID_SOCK)
    {
        CLOSE_SOCK(sock_);
        sock_ = INVALID_SOCK;
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
#endif

    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ == INVALID_SOCK)
        throw std::runtime_error("socket() failed");

    int enable = 1;

    // SO_BROADCAST
    if (setsockopt(sock_, SOL_SOCKET, SO_BROADCAST,
                   reinterpret_cast<const char*>(&enable), sizeof(enable)) == SOCK_ERROR)
    {
        std::cerr << "Warning: setsockopt(SO_BROADCAST) failed\n";
    }

    // SO_REUSEADDR
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&enable), sizeof(enable)) == SOCK_ERROR)
    {
        std::cerr << "Warning: setsockopt(SO_REUSEADDR) failed\n";
    }

    // Non-blocking mode
#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(sock_, FIONBIO, &mode) == SOCK_ERROR)
    {
        std::cerr << "Warning: ioctlsocket(FIONBIO) failed\n";
    }
#else
    int flags = fcntl(sock_, F_GETFL, 0);
    if (flags == -1 || fcntl(sock_, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Warning: fcntl(O_NONBLOCK) failed\n";
    }
#endif
}

std::optional<UDPError> UDPSocket::bind()
{
    reset();

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = intefaceIP_;
    addr.sin_port        = port_;   // предполагается, что уже в сетевом порядке (htons)

    if (::bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0)
        return std::nullopt;

    return last_udp_error();
}

std::optional<UDPError> UDPSocket::bind(uint16_t port)
{
    port_ = htons(port);
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

std::variant<size_t, UDPError> UDPSocket::send_to(const uint8_t* data, size_t size, uint32_t ip)
{
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port        = port_;

    int rc = sendto(sock_,
                    reinterpret_cast<const char*>(data),
                    static_cast<int>(size),
                    0,
                    reinterpret_cast<sockaddr*>(&addr),
                    sizeof(addr));

    if (rc >= 0)
        return static_cast<size_t>(rc);

    return last_udp_error();
}

std::variant<size_t, UDPError> UDPSocket::send_to(const uint8_t* data, size_t size, IPAddress ip)
{
    return send_to(data, size, ip.toNet());
}

std::variant<ReceiveInfo, UDPError> UDPSocket::recieve(uint8_t* buf, size_t size)
{
    sockaddr_in srcaddr{};
    socklen_t addrlen = sizeof(srcaddr);

    int rc = recvfrom(sock_,
                      reinterpret_cast<char*>(buf),
                      static_cast<int>(size),
                      0,
                      reinterpret_cast<sockaddr*>(&srcaddr),
                      &addrlen);

    if (rc >= 0)
    {
        if (srcaddr.sin_family == AF_INET)
        {
            IPAddress remote_ip = IPAddress::fromNet(srcaddr.sin_addr.s_addr);
            auto my_ips = intefacesIPs();

            if (std::find(my_ips.begin(), my_ips.end(), remote_ip) != my_ips.end())
                return RECEIVE_NONE;

            return ReceiveInfo(rc, remote_ip);
        }
        return ReceiveInfo(rc, IP_ANY);
    }

    UDPError err = last_udp_error();
    if (err == UDPError::WOULD_BLOCK)
        return RECEIVE_NONE;

    return err;
}

uint16_t UDPSocket::getBindPort()
{
    return ntohs(port_);
}

uint32_t UDPSocket::getBindInterface()
{
    return intefaceIP_;
}

// ────────────────────────────────────────────────
//  Получение списка IP-адресов интерфейсов
// ────────────────────────────────────────────────

#ifdef _WIN32

#include <iphlpapi.h>

std::vector<IPAddress> intefacesIPs()
{
    static auto last_update = std::chrono::steady_clock::now();
    static std::vector<IPAddress> cached_ips;

    auto now = std::chrono::steady_clock::now();
    if (now - last_update < std::chrono::seconds(10) && !cached_ips.empty())
        return cached_ips;

    ULONG bufLen = 15000;
    std::vector<uint8_t> buffer(bufLen);

    DWORD ret = GetAdaptersAddresses(AF_INET, 0, nullptr,
                                     reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()),
                                     &bufLen);

    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufLen);
        ret = GetAdaptersAddresses(AF_INET, 0, nullptr,
                                   reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()),
                                   &bufLen);
    }

    if (ret != NO_ERROR)
    {
        std::cerr << "GetAdaptersAddresses failed: " << ret << std::endl;
        return {};
    }

    std::vector<IPAddress> ips;
    auto* adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    while (adapter)
    {
        auto* unicast = adapter->FirstUnicastAddress;
        while (unicast)
        {
            if (unicast->Address.lpSockaddr->sa_family == AF_INET)
            {
                auto* sin = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
                ips.push_back(IPAddress::fromNet(sin->sin_addr.s_addr));
            }
            unicast = unicast->Next;
        }
        adapter = adapter->Next;
    }

    cached_ips = std::move(ips);
    last_update = now;
    return cached_ips;
}

#else

std::vector<IPAddress> intefacesIPs()
{
    static auto last_update = std::chrono::steady_clock::now();
    static std::vector<IPAddress> cached_ips;

    auto now = std::chrono::steady_clock::now();
    if (now - last_update < std::chrono::seconds(10) && !cached_ips.empty())
        return cached_ips;

    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0)
        return {};

    std::vector<IPAddress> ips;

    for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            auto* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            ips.push_back(IPAddress::fromNet(sin->sin_addr.s_addr));
        }
    }

    freeifaddrs(ifaddr);

    cached_ips = std::move(ips);
    last_update = now;
    return cached_ips;
}

#endif

// ────────────────────────────────────────────────
//  Обработка ошибок
// ────────────────────────────────────────────────

UDPError last_udp_error()
{
    int err = LAST_ERROR();

#ifdef _WIN32
    switch (err)
    {
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
        case WSAECONNRESET:         return UDPError::WOULD_BLOCK;  // типично для UDP — игнорируем
        case WSANOTINITIALISED:     return UDPError::WSA_NOT_INITIALIZED;
        default:                    return static_cast<UDPError>(err + 10000);
    }
#else
    switch (err)
    {
        case 0:                     return UDPError::SUCCESS;
        case EACCES:                return UDPError::PERMISSION_DENIED;
        case EMSGSIZE:              return UDPError::MESSAGE_TOO_LARGE;
        case ENOBUFS:               return UDPError::NO_BUFFER_SPACE;
        case EAGAIN:                return UDPError::WOULD_BLOCK;
        case EBADF:                 return UDPError::INVALID_SOCKET_DESC;
        case EINVAL:                return UDPError::INVALID_ARGUMENT;
        case ENETDOWN:              return UDPError::NETWORK_DOWN;
        case EADDRNOTAVAIL:         return UDPError::ADDRESS_NOT_AVAILABLE;
        case EOPNOTSUPP:            return UDPError::OPERATION_NOT_SUPPORTED;
        default:                    return static_cast<UDPError>(err);
    }
#endif
}

std::string udp_error_to_string(UDPError err)
{
    switch (err)
    {
        case UDPError::SUCCESS:               return "Success";
        case UDPError::PERMISSION_DENIED:     return "Permission denied";
        case UDPError::MESSAGE_TOO_LARGE:     return "Message too large";
        case UDPError::NO_BUFFER_SPACE:       return "No buffer space";
        case UDPError::WOULD_BLOCK:           return "Would block";
        case UDPError::INVALID_SOCKET_DESC:   return "Invalid socket descriptor";
        case UDPError::INVALID_ARGUMENT:      return "Invalid argument";
        case UDPError::NETWORK_DOWN:          return "Network down";
        case UDPError::ADDRESS_NOT_AVAILABLE: return "Address not available";
        case UDPError::OPERATION_NOT_SUPPORTED:return "Operation not supported";
        case UDPError::WSA_NOT_INITIALIZED:   return "Winsock not initialized";
        default:                              return "Unknown error: " + std::to_string(static_cast<int>(err));
    }
}
