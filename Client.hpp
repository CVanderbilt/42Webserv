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

#define TIMEOUT 30000 // in miliseconds

class Client
{
private:
	int							_status;
	int							_response_status;
	std::string					_response;
	size_t						_response_sent;
	size_t						_response_left;
	Http_req					_request;
	std::map<int, std::string>	_stat_msg;
	std::string					_redirect;
	uint64_t					_time_check;
	std::map<int, std::string>	*_error_pages;
	server_info 				*_s;

	typedef std::pair<const server_location*, std::string> LPair;

	std::string		ExecuteCGI(const server_location *s);
	int				ResponseStatus(const server_location *);
	bool			MethodAllowed() const;
	void			CheckCGIHeaders(std::string headers);
	std::string		BuildError();
	std::string		GetFile(LPair& lpair);
	std::string		GetIndex(LPair& lpair, std::string& directory);
	std::string		BuildGet(LPair& lpair);
	std::string		BuildPost(LPair& lpair);
	std::string		BuildDelete(LPair& lpair);
	std::string		BuildAutoindex();
	std::string		GetAutoIndex(const std::string& directory, LPair& lpair);
	std::string		WrapHeader(const std::string& msg, const server_location *);
	bool			isCGI(const server_location *) const;
	std::map<int, std::string>		StatusMessages();
	std::string		lastModified(const server_location *) const;
	LPair 			locationByUri(const std::string& uri, const std::vector<server_location>& locs) const;
	std::string		setContentType() const;

public:

	Client();
	Client(Client const &copy);
	virtual ~Client(){};

	void			setServer(server_info *s);
	int 			getStatus() const;
	void			updateTime();
	int				getParseChunk(char *chunk, size_t bytes);
	void			BuildResponse();
	std::string		getResponse() const;
	size_t			getResponseSent() const;
	void			setResponseSent(size_t sent);
	size_t			getResponseLeft() const;
	void			setResponseLeft(size_t left);
	const Http_req&	GetRequest() const;
	void 			reset();
	bool			hasTimedOut();

};