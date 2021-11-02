#include "Client.hpp"

Client::Client(int fd) : 
	status(-1),
	_is_sending(0),
	_fd(fd)
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