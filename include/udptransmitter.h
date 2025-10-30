#if !defined UDP_TRANSMITTER_H
#define UDP_TRANSMITTER_H

#include <udpsocket.h>



class UDPTransmitter 
{
	std::variant<UDPSocket, const UDPSocket*> sock_;
	uint16_t port_;
	IPAddress target_;
	
	bool lockTargetIP_;
	std::string magicString_;
public:
	UDPTransmitter(uint16_t port, std::string magicString) :
	sock_(), port_(port), target_(IP_BROADCAST), magicString_(std::move(magicString))
	{}

	UDPTransmitter(uint16_t port, std::string magicString, UDPSocket sock) :
	sock_(sock), port_(port), target_(IP_BROADCAST), magicString_(std::move(magicString))
	{}


	void setLockTargetIP(bool lock)
	{
		lockTargetIP_ = lock;
	}
	bool getLockTargetIP()
	{
		return lockTargetIP_;
	}
	bool lockTargetIP()
	{
		return lockTargetIP_;
	}
	void lockTargetIP(bool lock)
	{
		lockTargetIP_ = lock;
	}

	void setTargetIP(IPAddress targetIP, bool lockTargetIP = true)
	{
		lockTargetIP_ = lockTargetIP;
		target_ = targetIP;
	}

	void setBroadcastTargetIP()
	{
		setTargetIP(IP_BROADCAST);
	}

	void resetTargetIP()
	{
		setTargetIP(IP_BROADCAST, false);
	}

	bool isValid() { return true; } // This method is not necessary, it is needed for better compatibility with the original library.

	int sendData(const uint8_t* data, size_t dataSize);

	int sendData(const char* data);

	template <size_t N>
	int sendData(const Message<N>& data);

	ReceiveInfo receiveData(uint8_t* buffer, size_t maxSize);

	template <size_t N>
	ReceiveInfo receiveData(Message<N>* buffer);

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