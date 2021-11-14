#include <vector>
#include <iostream>
#include <poll.h>
#include "Config.hpp"
#include "Server.hpp"

int main(int argc, char *argv[])
{
	
	std::vector<server_config> config;
	if (argc != 2)
		std::cerr << "Wrong number of arguments" << std::endl;
	try
	{
		config = check_config(argv[1]);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	server_config c;
	if (config.size() == 0)
	{
		std::cout << "server config vector empty" << std::endl;
		return (1);
	}
	std::cout << "----------------------------------------------------------------------------------------" << std::endl;
	for (std::vector<server_config>::iterator it = config.begin(); it != config.end(); it++)
	{
		c = *it;
		std::cout << "server configuration:" << std::endl;
		std::cout << "  options:" << std::endl;
		for (std::map<std::string, std::string>::iterator mit = c.opts.begin(); mit != c.opts.end(); mit++)
			std::cout << "    " << mit->first << ": " << mit->second << std::endl;
		std::cout << "  locations:" << std::endl;
		std::cout << "    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
		for (std::vector<location_config>::iterator cit = c.loc.begin(); cit != c.loc.end(); cit++)
		{
			std::cout << "    location configuration:" << std::endl;
			std::cout << "      path: " << cit->path << std::endl;
			std::cout << "      options:" << std::endl;
			for (std::map<std::string, std::string>::iterator lmit = cit->opts.begin(); lmit != cit->opts.end(); lmit++)
				std::cout << "        key: " << lmit->first << ": " << lmit->second << std::endl;
			std::cout << "    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
		}
		std::cout << "----------------------------------------------------------------------------------------" << std::endl;
	}

	std::vector<Server*>servers;
	for (std::vector<server_config>::const_iterator it = config.begin(); it != config.end(); it++)
	{
		servers.push_back(new Server(*it));
		servers[servers.size() - 1]->server_start();
	}

	size_t idx = 0;
	while (1)
	{
//		std::cout << "server: " << idx << std::endl;
		servers[idx]->server_listen();
		idx += 1;
		idx %= servers.size();
//		std::cout << "index++" << std::endl;
	}
}
