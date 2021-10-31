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
#include "Http_req.hpp"

class Client
{
private:
	bool		_is_sending;
	int			_fd;
	Client();
	
public:

	int			status;
	Http_req	request;
	Client(int const fd);
	virtual ~Client(){};

	int		getFd() const;
	void	setFd(int const &fd);
};