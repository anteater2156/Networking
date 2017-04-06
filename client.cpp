#include "sockethandler.h"
#include "debug.h"

#include <vector>
#include <iostream>

int main (int argc, char *argv[]) 
{
	std::cout << "The client is up and running.\n";

	TCPClientHandler tHandler;
	if ( tHandler.connectSocket("10.10.14.18", "4446") == false)
	{
		std::cout << "Could not connect to socket\n";
		exit(1);
	}

	std::cout << "The client has successfully connected to server 10.10.14.18\n";

	tHandler.kill();

	return 0;
}