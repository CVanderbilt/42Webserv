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
	int			_status;
	Http_req	_request;
	std::string	_response;
	size_t		_response_sent;
	size_t		_response_left;
	
public:

	Client();
	Client(int const fd);
	Client(Client const &copy);
	virtual ~Client(){};

	int			getFd() const;
	void		setFd(int const &fd);
	int 		getStatus();
	void		getParseChunk(std::string chunk);
	std::string	getResponse();
	size_t		getResponseSent();
	void		setResponseSent(size_t sent);
	size_t		getResponseLeft();
	void		setResponseLeft(size_t left);
};