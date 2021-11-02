#include "Server.hpp"

int		main()
{
	try
	{
		Server my_server;
		my_server.server_start();
		while(1)
			my_server.server_listen();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}