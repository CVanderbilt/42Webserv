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
#include <sstream>
#include <cerrno>
#include <sys/types.h>
#include <dirent.h>
#include "Http_req.hpp"
#include "utils.hpp"
#include "CGI.hpp"

#define TIMEOUT 3000 // in miliseconds

class Client
{
private:
	int							_fd;
	int							_status;
	int							_response_status;
	std::string					_response;
	std::string					_response_cgi;
	size_t						_response_sent;
	size_t						_response_left;
	Http_req					_request;
	std::map<int, std::string>	_stat_msg;
	bool						_is_autoindex;
	std::string					_req_file;
	std::string					_redirect;
	uint64_t					_time_check;
	std::map<int, std::string>	*_error_pages;
	server_info 				*_s;

	std::string		ExecuteCGI(const server_location *s);
public:

	Client();
	Client(int const fd);
	Client(Client const &copy);
	virtual ~Client(){};

	void		setServer(server_info *s);
	int			getFd() const;
	void		setFd(int const &fd);
	int 		getStatus();
	void		getParseChunk(char *chunk, size_t bytes);
	void		BuildResponse();
	std::string	getResponse();
	size_t		getResponseSent();
	void		setResponseSent(size_t sent);
	size_t		getResponseLeft();
	void		setResponseLeft(size_t left);
	int			ResponseStatus(const server_location *);
	bool		MethodAllowed();
	std::string	BuildGet(const server_location *);
	std::string	BuildPost(const server_location *);
	std::string	BuildDelete(const server_location *);
	std::string	BuildAutoindex();
	std::string GetAutoIndex(const std::string& directory, const std::string& url_location);
	std::string	WrapHeader(const std::string& msg, const server_location *);
	std::map<int, std::string>		StatusMessages();
	const Http_req&	GetRequest();
	void 		reset();
	bool		isCGI(const server_location *);
	bool		hasTimedOut();
	std::string	lastModified(const server_location *);
	std::string	setContentType();
};