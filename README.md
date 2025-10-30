## EasyUDPLib - библиотека для передачи данных между устройствами, аналог [EasyEthernetLib](https://github.com/iliangood/EasyEthernetLib) для операционных систем(на основе Linux, Windows)
> [!WARNING]
> Эта библиотека не гарантирует передачу данных и их целостность, она предназначена для передачи потоковых данных, например сигналы управления с пульта и данные с датчиков с аппарата

### Установка на Linux:
```bash
git clone https://github.com/iliangood/EasyUDPLib.git
mkdir EasyUDPLib/build
cd EasyUDPLib/build
cmake ..
sudo make install 
```

## Использование

### CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.28.3)
project(test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(UDPLibrary CONFIG REQUIRED)

add_executable(prog main.cpp)

target_link_libraries(prog PUBLIC UDPLibrary::udp_library)
```
### main.cpp:
```cpp
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
		msg.clear();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
    
    return 0;
}
```
