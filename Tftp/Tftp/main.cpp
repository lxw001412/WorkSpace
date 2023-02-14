#include"CTftp.h"
#include <iostream>
int main()
{
	int i = 0;
	MyTftp* tftp = new MyTftp("D:\\gitdown");
	tftp->Start();
	while (1)
	{
		std::cout << i++ << std::endl;
		Sleep(400);
	}
	return 0;
}