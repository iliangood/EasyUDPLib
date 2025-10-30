#include <iostream>
#include <thread>
#include <string>
#include <chrono>

#include <udptransmitter.h>

struct exempleStruct
{
	int a;
	long long b;
};

int main()
{
    UDPTransmitter transmitter(45088, "testing");
    
    Message<1024> msg;

	while(true)
	{
		msg.push("можно записать строку");
		msg.push((uint32_t)12542);
		msg.push(exempleStruct{124, 15125});
		transmitter.sendData(msg);

		msg.clear();
		ReceiveInfo rc = transmitter.receiveData(&msg);
		if(recieved(rc))
		{
			std::cout << "recieved from: ";
			if(rc.remoteIP.has_value())
				std::cout << rc.remoteIP.value() << std::endl;
			else
				std::cout << "unkown" << std::endl;
			std::cout << "data:" << std::endl;
			std::cout << msg.readString() << std::endl;
			std::cout << msg.read<uint32_t>() << std::endl;
			exempleStruct ex = msg.read<exempleStruct>();
			std::cout << ex.a << std::endl;
			std::cout << ex.b << std::endl;
		}
	}
    
    return 0;
}