#include <iostream>
#include <thread>
#include "../LogicLib/ServerHost.h"

int main()
{
	LogicLib::ServerHost ServerHost;
	ServerHost.Init();

	std::thread logicThread([&]() { 		
		ServerHost.Run(); }
	);
	
	std::cout << "press any key to exit...";
	getchar();

	ServerHost.Stop();
	logicThread.join();

	return 0;
}