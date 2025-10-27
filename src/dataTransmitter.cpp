#include <dataTransmitter.h>

DataTransmitter::DataTransmitter(uint16_t port, const char* magicString)
{
#ifdef _WIN32
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		std::cerr << "WSAStartup failed." << std::endl;
		throw std::runtime_error("DataTransmitter::DataTransmitter(uint16_t, const char*) WSAStartup failed");
		return;
	}

	sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_ == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		throw std::runtime_error("DataTransmitter::DataTransmitter(uint16_t, const char*) Error creating socket");
		return;
	}
#else
	sock_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_ < 0) {
		std::cerr << "Error creating socket" << std::endl;
		throw std::runtime_error("DataTransmitter::DataTransmitter(uint16_t, const char*) Error creating socket");
		return;
	}
#endif

	target_.sin_family = AF_INET;
	target_.sin_port = htons(port);
	target_.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	if(magicString != nullptr)
	{
		magicStringLength_ = strlen(magicString);
		magicString_ = (char*)malloc(magicStringLength_*sizeof(char));
		memcpy(magicString_, magicString, magicStringLength_ + 1);
		magicString_[magicStringLength_] = 0;
	}
	else
	{
		magicStringLength_ = 0;
		magicString_ = nullptr;
	}
}

DataTransmitter::~DataTransmitter()
{
	free(magicString_);
#ifdef _WIN32
	closesocket(sock_);
	WSACleanup();
#else
	close(sock_);
#endif
}

void DataTransmitter::setLockTargetIP(bool lock)
{
	lockTargetIP_ = lock;
}

bool DataTransmitter::getLockTargetIP()
{
	return lockTargetIP_;
}

bool DataTransmitter::lockTargetIP()
{
	return lockTargetIP_;
}

void DataTransmitter::setTargetIP(in_addr_t targetIP, bool lockTargetIP)
{
	setLockTargetIP(lockTargetIP);
	target_.sin_addr.s_addr = htonl(targetIP);
}

void setBroadcastTargetIP();