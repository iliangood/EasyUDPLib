#include <iostream>
#include <thread>
#include <string>

#include <udptransmitter.h>

int main()
{
	UDPTransmitter transmitter(45088, "testing");
	uint8_t buf[1024];
	transmitter.bindInterface(IPAddress(192,168,1,2));
	while (true)
	{
		transmitter.sendData("hello from PC");
		std::cout << "sended" << std::endl;
		ReceiveInfo rc = transmitter.receiveData(buf, 1024);
		if(recieved(rc))
		{
			std::cout << "recieved 1:" << buf << std::endl;
			if(rc.remoteIP.has_value())
				std::cout << "from: " << rc.remoteIP.value() << std::endl;
			else
				std::cout << "from: unknown" << std::endl;
		}
		rc = transmitter.receiveData(buf, 1024);
		if(recieved(rc))
		{
			std::cout << "recieved 2:" << buf << std::endl;
			if(rc.remoteIP.has_value())
				std::cout << "from: " << rc.remoteIP.value() << std::endl;
			else
				std::cout << "from: unknown" << std::endl;
		}
		rc = transmitter.receiveData(buf, 1024);
		if(recieved(rc))
		{
			std::cout << "recieved 3:" << buf << std::endl;
			if(rc.remoteIP.has_value())
				std::cout << "from: " << rc.remoteIP.value() << std::endl;
			else
				std::cout << "from: unknown" << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	
	return 0;
}