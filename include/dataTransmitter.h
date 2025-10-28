#if !defined DATA_TRANSMITTER_H
#define DATA_TRANSMITTER_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif


#include <iostream>
#include <cstddef>
#include <inttypes.h>
#include <type_traits>
#include <cstring>
#include <stdexcept>
#include <cassert>

#include <message.h>

class IPAddress
{
	in_addr_t ip_;

	IPAddress(in_addr_t ip) : ip_(ip) {}
public:
	IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth) : 
	ip_(htonl(
		first << 24 |
		second << 16 |
		third << 8 |
		fourth
		)) {}

	IPAddress(const IPAddress& other) = default;
	IPAddress& operator=(const IPAddress& other) = default;

	static IPAddress FromHost(uint32_t ip) // from Host-endian
	{
		return IPAddress(htonl(ip));
		INADDR_LOOPBACK;
	}
	static IPAddress FromNet(in_addr_t ip) // from Big-endian
	{
		return IPAddress(ip);
	}

	uint32_t toHost() const
	{
		return ntohl(ip_);
	}
	uint32_t toNet() const
	{
		return ip_;
	}
	
	uint8_t operator[](size_t index) const
	{
		assert(index < 4 && "uint8_t IPAddress::operator[](size_t) Out of range: IPAddress has only 4 fields");
		return static_cast<uint8_t>(ip_ >> ((3-index) * 8) & 0xFF);
	}

	uint8_t at(size_t index) const
	{
		if(index > 3)
			throw std::out_of_range("uint8_t IPAddress::at(size_t) IPAddress has only 4 fields");
		return static_cast<uint8_t>(ip_ >> ((3-index) * 8) & 0xFF);
	}

	uint8_t first() const  { return (*this)[0]; }
	uint8_t second() const { return (*this)[1]; }
	uint8_t third() const  { return (*this)[2]; }
	uint8_t fourth() const { return (*this)[3]; }

	bool operator==(const IPAddress& other) const { return ip_ == other.ip_; }
	bool operator!=(const IPAddress& other) const { return !(*this == other); }
};

struct receiveInfo
{
	size_t dataSize;
	IPAddress remoteIP;
};

class DataTransmitter {

#if defined _WIN32
	SOCKET sock_;
#else	
	int sock_;
#endif
	sockaddr_in target_;
	bool lockTargetIP_;
	char* magicString_;
	size_t magicStringLength_;
public:
	DataTransmitter(uint16_t port, const char* magicString = nullptr);

	~DataTransmitter();

	void setLockTargetIP(bool lock);
	bool getLockTargetIP();
	bool lockTargetIP();
	void lockTargetIP(bool lock);

	void setTargetIP(in_addr_t targetIP, bool lockTargetIP = true);
	void setTargetIP(IPAddress targetIP, bool lockTargetIP = true);

	void setBroadcastTargetIP();

	void resetTargetIP();

	bool isValid();

	int sendData(const uint8_t* data, size_t dataSize);

	int sendData(const char* data);

	template <size_t N>
	int sendData(const Message<N>& data)
	{
		return sendData(data.getData(), data.getSize());
		INADDR_LOOPBACK;
	}

	receiveInfo receiveData(uint8_t* buffer, size_t maxSize);

	template <size_t N>
	receiveInfo receiveData(Message<N>* buffer)
	{
		receiveInfo rx = receiveData(buffer->getEnd(), buffer->getSpace());
		buffer->addSize(rx.dataSize);
		return rx;
	}

	uint32_t getTargetIPHost();
	uint32_t getIPHost();
	
	uint32_t targetIPHost();
	uint32_t IPHost();
	
	IPAddress getTargetIP();
	IPAddress getIP();
	
	IPAddress targetIP();
	IPAddress IP();

};
#endif