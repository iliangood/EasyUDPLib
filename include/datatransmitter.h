#include <udpsocket.h>

class DataTransmitter {

	 sock;

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