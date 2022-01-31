#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <fstream>
#include <iostream> 
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <map>
#include "Http_req.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "utils.hpp"

#define BUFFER_SIZE 4096
#define MAX_CONNEC 50
class Server
{
private:

	int								_server_fd;
	size_t							_fd_count;
	size_t							_fd_size;
	sockaddr_in						_addr;
	socklen_t						_addrlen;
	pollfd 							*_pfds;
	std::map<int, Client>			_clients;
	server_info						_configuration;

	void			pollin_handler(int i);
	int				pollout_handler(int i);
	void			accept_connection();
	void			read_message(int i);
	void			add_to_pfds(int new_fd);
	void			del_from_pfds(int i);
	void			close_fd_del_client(int i);
	int				send_response(int i);
	void			addServerConfig(server_config const& s);
	void			addServerLocations(server_config const& s);
	Server(){};

public:

	Server(server_config const& s, std::map<std::string, std::string> *cgi_exec_path);
	Server(Server const &copy);
	virtual ~Server() {};
	void			server_start();
	void			server_listen();

	class ServerException : public std::exception
	{
	private:
		std::string		_error;
	
	public:
		ServerException(void);
		ServerException(std::string function, std::string error);
		virtual				~ServerException(void) throw() {};
		virtual const char* what() const throw();
	};
};