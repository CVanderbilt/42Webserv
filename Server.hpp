#pragma once

#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <fstream> // For files
#include <iostream> 
#include <unistd.h> // For read
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <map>
#include "Http_req.hpp"
#include "Client.hpp"
#include <utility>

#define PORT 9999
#define BUFFER_SIZE 4096
#define MAX_CONNEC 20

class Server
{
private:

	int						_server_fd;
	int						_fd_count;
	int						_fd_size;
	sockaddr_in				_addr;
	socklen_t				_addrlen;
	pollfd 					*_pfds;
	std::map<int, Client>	_clients;

	void			accept_connection();
	void			read_message(int i);
	void			add_to_pfds(int new_fd);
	void			del_from_pfds(int fd, int i);

public:

	Server(void);
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