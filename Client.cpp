#include "Client.hpp"


Client::Client() : 
	_status(-1),
	_is_sending(0),
	_fd(-1)
{}

Client::Client(int fd) : 
	_status(-1),
	_is_sending(0),
	_fd(fd)
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