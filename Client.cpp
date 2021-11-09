#include "Client.hpp"


Client::Client() : 
	_status(-1),
	_is_sending(0),
	_fd(-1),
	_response_sent(0),
	_response_left(0)
{}

Client::Client(int fd) : 
	_status(-1),
	_is_sending(0),
	_fd(fd),
	_response_sent(0),
	_response_left(0)
{}

Client::Client(Client const &copy) :

	_status( copy._status),
	_is_sending(copy._is_sending),
	_fd(copy._fd)
{} 

int		Client::getFd() const
{
	return (_fd);
}

void	Client::setFd(int const &fd)
{
	_fd = fd;
	return;
}

int		Client::getStatus()
{
	return (_status);
}

void	Client::getParseChunk(std::string chunk)
{
	Http_req::parsing_status temp;

	if ((temp = _request.parse_chunk(chunk)) == Http_req::PARSE_ERROR)
		_status = 0;
	else if (temp == Http_req::PARSE_END)
		_status = 1;
}

std::string	Client::getResponse()
{
	return (_response);
}

size_t	Client::getResponseSent()
{
	return (_response_sent);
}

void	Client::setResponseSent(size_t sent)
{
	_response_sent = sent;
}

size_t	Client::getResponseLeft()
{
	return (_response_left);
}

void	Client::setResponseLeft(size_t left)
{
	_response_left = left;
}

int		Client::setRespStatus()
{
	if (_status == 1)
	{
		if (_request.protocol.compare("HTTP/1.1") != 0)
			return (_response_status = 505);
		else
			return (_response_status = 400);
	}
	if (MethodAllowed(_request.method) == false)
		return (_response_status = 405);
	return (200);
}

bool	Client::MethodAllowed(std::string method)
{
	if (_request.method.compare("GET") == 0 ||
		_request.method.compare("POST") == 0 ||
		_request.method.compare("DELETE") == 0)
		return (true);
	else
		return (false);
}