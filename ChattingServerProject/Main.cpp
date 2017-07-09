#include <iostream>
#include <thread>
#pragma comment(lib, "LogicLib")
#pragma comment(lib, "NetworkLib")
#include "../LogicLib/LogicLib/Main.h"

int main()
{
	LogicLib::Main main;

	main.Init();

	main.Run();

	std::cout << "press any key to exit...";
	getchar();

	main.Stop();

	return 0;
}