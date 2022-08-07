#include <iostream>
#include <thread>

#include "Server.h"

int main()
{
	Server server;
	server.Init(); // 내부적으로 TcpNetwork에서의 Init()를 호출 (처음 한번만 호출하면됨)

	std::thread runThread([&]() { // Run() 함수가 이벤트를 감지할때까지 기다리기만 하기때문에 멀티쓰레드로 호출
		server.Run(); // 여기도 내부적으로 TcpNetwork에서의 Run()을 호출
	});

	std::cout << "press any key to exit...";
	getchar(); // 바로 서버가 종료되는것을 방지

	server.Stop(); // 서버 종료
	runThread.join(); // 쓰레드도 종료되길 기다림

	return 0;
}