#include <iostream>
#include <thread>
#include <string>
#include <chrono>

#include <udptransmitter.h>

int main()
{
    UDPTransmitter transmitter(45088, "testing");
    uint8_t buf[1024];
    
    try {
        transmitter.setBroadcastTargetIP();
        
        std::cout << "Bind interface: " << transmitter.getBindInterface() << std::endl;
        std::cout << "Bind port: " << transmitter.getBindPort() << std::endl;
        
        std::cout << "Available interfaces:" << std::endl;
        std::vector<IPAddress> IPs = intefacesIPs();
        for(const IPAddress& ip : IPs)
        {
            std::cout << "  " << ip << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return 1;
    }

    while (true)
    {
        // Отправка данных
        try {
            transmitter.sendData("hello from PC");
            std::cout << "Sent data successfully" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Send error: " << e.what() << std::endl;
        }

        // Попытка приёма данных с таймаутом
        auto start = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(1000); // 1 секунда на приём

        while (std::chrono::steady_clock::now() - start < timeout)
        {
            ReceiveInfo rc = transmitter.receiveData(buf, 1024);
            if(recieved(rc))
            {
                std::cout << "Received: " << buf << std::endl;
                if(rc.remoteIP.has_value())
                    std::cout << "From: " << rc.remoteIP.value() << std::endl;
                else
                    std::cout << "From: unknown" << std::endl;
                break; // Выходим после успешного приёма
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
	
	return 0;
}