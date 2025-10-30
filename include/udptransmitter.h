#if !defined UDP_TRANSMITTER_H
#define UDP_TRANSMITTER_H

#include <iostream>
#include <cstring>

#include <udpsocket.h>



class UDPTransmitter 
{
	std::variant<UDPSocket, UDPSocket*> sock_;
	IPAddress target_;
	
	bool lockTargetIP_;
	std::string magicString_;

	UDPSocket& sock()
	{
		if(std::holds_alternative<UDPSocket>(sock_))
			return std::get<UDPSocket>(sock_);
		return *std::get<UDPSocket*>(sock_);
	}
public:
	UDPTransmitter(uint16_t port, std::string magicString) :
	sock_(), target_(IP_BROADCAST), magicString_(std::move(magicString))
	{
		sock().bind(hton(port));
		sock().bindInteface(IP_ANY);
	}

	UDPTransmitter(UDPSocket* sock, std::string magicString) :
	sock_(sock), target_(IP_BROADCAST), magicString_(std::move(magicString))
	{
		this->sock().bindInteface(target_);
		this->sock().bindInteface(IP_ANY);
	}

	uint16_t getBindPort()
	{
		return ntoh(sock().getBindPort());
	}

	IPAddress getBindInterface()
	{
		return IPAddress::fromNet(sock().getBindInterface());
	}

	bool bind(uint32_t port) // host-endian, returns true if success
	{
		std::optional<UDPError> rc = sock().bind(hton(port));
		if(!rc.has_value())
			return true;
		std::cerr << udp_error_to_string(rc.value()) << std::endl;
		return false;
	}

	bool bindInterface(IPAddress ip) // returns true if success
	{
		std::optional<UDPError> rc = sock().bindInteface(ip);
		if(!rc.has_value())
			return true;
		std::cerr << udp_error_to_string(rc.value()) << std::endl;
		return false;
	}


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

	ssize_t sendData(const uint8_t* data, size_t dataSize)
	{
		uint8_t* buf = new uint8_t[dataSize + magicString_.length()];
		memcpy(buf, magicString_.c_str(), magicString_.length());
		memcpy(buf + magicString_.length(), data, dataSize);
		std::variant<size_t, UDPError> rc = sock().send_to(buf, dataSize + magicString_.length(), target_);
		if(std::holds_alternative<UDPError>(rc))
		{
			std::cerr << udp_error_to_string(std::get<UDPError>(rc)) << std::endl;
			return -1;
		}
		return std::get<size_t>(rc);
	}

	ssize_t sendData(const char* data)
	{
		size_t length = strlen(data);
		return sendData(reinterpret_cast<const uint8_t*>(data), length+1);
	}

	template <size_t N>
	ssize_t sendData(const Message<N>& data)
	{
		return sendData(data.data(), data.size());
	}

	ReceiveInfo receiveData(uint8_t* buffer, size_t maxSize)
	{
		std::variant<ReceiveInfo, UDPError> rc = sock().recieve(buffer, maxSize);
		if(std::holds_alternative<UDPError>(rc))
		{
			std::cerr << udp_error_to_string(std::get<UDPError>(rc)) << std::endl;
			return ReceiveInfo(0, std::nullopt);
		}
		if(std::get<ReceiveInfo>(rc).dataSize < magicString_.length())
			return RECEIVE_NONE;
		if(memcmp(magicString_.c_str(), buffer, magicString_.length()) != 0)
			return RECEIVE_NONE;
		std::optional<IPAddress> remoteIP = std::get<ReceiveInfo>(rc).remoteIP;
		if(remoteIP.has_value())
		{
			if(target_ != remoteIP.value())
			{
				if(lockTargetIP_ && target_ != IP_BROADCAST)
					return RECEIVE_NONE;
				target_ = remoteIP.value();
			}

		}
		size_t new_size = std::get<ReceiveInfo>(rc).dataSize - magicString_.length();
		memmove(buffer, buffer + magicString_.length(), new_size);
		return ReceiveInfo(new_size, remoteIP);
	}

	template <size_t N>
	ReceiveInfo receiveData(Message<N>* buffer)
	{
		ReceiveInfo rc = receiveData(buffer->size(), buffer->space());
		buffer->addSize(rc.dataSize);
		return rc;
	}

	uint32_t getTargetIPHost() const
	{
		return target_.toHost();
	}
	
	uint32_t targetIPHost() const
	{
		return getTargetIPHost();
	}
	
	IPAddress getTargetIP() const
	{
		return target_;
	}
	
	IPAddress targetIP() const
	{
		return getTargetIP();
	}
};

#endif