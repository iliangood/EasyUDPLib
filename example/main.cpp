#include <thread>

#include <udptransmitter.h>

int main()
{
	UDPTransmitter transmitter(45088, "testing");
	while (true)
	{
		transmitter.sendData("hello from PC");
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	
	return 0;
}