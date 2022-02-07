#include "Config.hpp"
#include "Server.hpp"

int main(int argc, char *argv[])
{
	std::vector<server_config> config;
	std::string config_file = "default.conf";
	std::map<std::string, std::string>	cgi_exec_path;

	if (argc > 2)
	{
		std::cerr << "Wrong number of arguments" << std::endl;
		std::cerr << "Usage: " << argv[0] << std::endl << " or " << std::endl << argv[0] << " [config path]" << std::endl;
		return (1);
	}
	try
	{
		if (argc == 2)
			config_file = argv[1];
		config = check_config(config_file, cgi_exec_path);
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
	try
	{
		std::vector<Server*> servers;
		for (std::vector<server_config>::const_iterator it = config.begin(); it != config.end(); it++)
		{
			servers.push_back(new Server(*it, &cgi_exec_path));
			servers[servers.size() - 1]->server_start();
		}

		size_t idx = 0;
		while (1)
		{
			servers[idx]->server_listen();
			idx += 1;
			idx %= servers.size();
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	return (0);
}
