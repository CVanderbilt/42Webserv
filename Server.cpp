#include "Server.hpp"

Server::Server() :
	_addrlen(sizeof(_addr)),
	_fd_size(2),
	_pfds(new pollfd[_fd_size]),
	_port(8080)
{}

Server::Server(int port) :
	_addrlen(sizeof(_addr)),
	_fd_size(2),
	_pfds(new pollfd[_fd_size]),
	_port(port)
{}

void	Server::server_start()
{
	if ((_server_fd = socket(PF_INET, SOCK_STREAM, 0)) == 0)
		throw ServerException("In accept", "failed for some reason");
	if ((fcntl(_server_fd, F_SETFL, O_NONBLOCK)) == -1)
	{
		close (_server_fd);
		throw ServerException("In fcntl", "failed for some reason");
	}
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(_port);
	_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(_addr.sin_zero, '\0', sizeof(_addr.sin_zero));
	if (bind(_server_fd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
	{
		close (_server_fd);
		throw ServerException("In bind", "failed for some reason");
	}
	if (listen(_server_fd, 10) < 0)
	{
		close (_server_fd);
		throw ServerException("In listen", "failed for some reason");
	}
	_pfds[0].fd = _server_fd;
	_pfds[0].events = POLLIN | POLLOUT;
	_fd_count = 1;
}

void	Server::accept_connection()
{
	int		new_fd;

	if((new_fd = accept(_server_fd, (struct sockaddr *)&_addr, &_addrlen)) < 0)
	{
		close(_server_fd);
		throw ServerException("In accept", "failed for some reason");
	}
	if ((fcntl(_server_fd, F_SETFL, O_NONBLOCK)) == -1)
	{
		close (new_fd);
		throw ServerException("In fcntl", "failed for some reason");
	}
	if(_fd_count < MAX_CONNEC)
	{
		Client new_client(new_fd);

		add_to_pfds(new_fd);
		_clients[new_fd] = new_client;
		std::cout << "server: new connection on socket " << new_fd << std::endl;
	}
	else
	{
		std::cout << "server: connection not accepted" << std::endl;
		/*TODO: send response to the client rejecting connection */
		close(new_fd);
	}
}

void	Server::read_message(int i)
{
	char	*buffer = new char[BUFFER_SIZE + 1];
	int 	numbytes;
	
//	std::cout << "leyendo del fd = " << _pfds[i].fd << std::endl;
	if ((numbytes = recv(_pfds[i].fd, buffer, BUFFER_SIZE, 0)) < 0)
	{
		close_fd_del_client(i);
		std::cout << "server: recv error" << std::endl;
	}
	else if (numbytes == 0)
	{
		close_fd_del_client(i);
		std::cout << "server: client closed connection" << std::endl;
	}
	else
	{
		buffer[numbytes] = '\0';
		if (_clients.count(_pfds[i].fd))
			_clients[_pfds[i].fd].getParseChunk(buffer);
		std::cout << "server: message read and parsed on socket " << _pfds[i].fd << std::endl;
	}
}

void	Server::server_listen()
{
	
	int poll_count = poll(_pfds, _fd_count, -1);
	if (poll_count == -1)
	{
		close(_server_fd);
		throw ServerException("In poll", "failed for some reason");
	}
	for (int i = 0; i < _fd_count; i++)
	{
		if (_pfds[i].revents & POLLIN)
		{
//			std::cout << "estamos en POLLIN en i = " << i << std::endl;
			if (_pfds[i].fd == _server_fd)
				accept_connection();
			else
				read_message(i);
		}		
		else if(_pfds[i].revents & POLLOUT)
		{
//			std::cout << "estamos en POLLOUT en i = " << i << std::endl;
			int status;
			if (_clients.count(_pfds[i].fd))
				status = _clients[_pfds[i].fd].getStatus();
			if (status > 0)
				send_response(i);
			else if (status == 0)
				std::cout << "Parse error" << std::endl;
			/*TODO: enviar al cliente página de error*/
			close_fd_del_client(i);
		}
		if(_pfds[i].fd == -1)
		{
			del_from_pfds(_pfds[i].fd, i);
			i--;
		}
	}
}

void	Server::add_to_pfds(int new_fd)
{
//	std::cout << "_fd_size = " << _fd_size << std::endl;
	if (_fd_count == _fd_size)
	{
		_fd_size = 2 * _fd_size > MAX_CONNEC? MAX_CONNEC : _fd_size * 2;
//		std::cout << "_fd_size = " << _fd_size << std::endl;
		pollfd	*temp = new pollfd[_fd_size];
		for (size_t i = 0; i < _fd_count; i++)
			temp[i] = _pfds[i];
		delete[] _pfds;
		_pfds = temp;		
	}
	_pfds[_fd_count].fd = new_fd;
	_pfds[_fd_count].events = POLLIN | POLLOUT;
	_fd_count++;
}

void	Server::del_from_pfds(int fd, int i)
{
	_pfds[i] = _pfds[_fd_count];
	_fd_count--;
}

void	Server::close_fd_del_client(int i)
{
	close(_pfds[i].fd);
	_clients.erase(_pfds[i].fd);
	_pfds[i].fd = -1;
}

void	Server::send_response(int i)
{
	size_t val_sent;
	const std::string response = "HTTP/1.1 200 OK\r\n"
							"Date: Sun, 18 Oct 2009 10:47:06 GMT\r\n"
							"Server: Apache/2.2.14 (Win32)\r\n"
							"Last-Modified: Sat, 20 Nov 2004 07:16:26 GMT\r\n"
							"ETag: \"10000000565a5-2c-3e94b66c2e680\"\r\n"
							"Accept-Ranges: bytes\r\n"
							"Content-Length: 44\r\n"
							"Keep-Alive: timeout=5, max=100\r\n"
							"Connection: Keep-Alive\r\n"
							"Content-Type: text/html\r\n"
							"\r\n" 
							"<html><body><h1>It works!</h1></body></html>";

	if ((val_sent = send(_pfds[i].fd, response.c_str(), response.length(), 0)) < 0)
		std::cout << "server: error sending response on socket " << _pfds[i].fd << std::endl;
	else if (val_sent < _clients[_pfds[i].fd].getResponseLeft())
	{
		_clients[_pfds[i].fd].setResponseSent(_clients[_pfds[i].fd].getResponseLeft() + val_sent);
		_clients[_pfds[i].fd].setResponseLeft(_clients[_pfds[i].fd].getResponse().length() - _clients[_pfds[i].fd].getResponseSent());
	}
	else
	{
		std::cout << "server: response sent on socket " << _pfds[i].fd << std::endl;
		_clients[_pfds[i].fd].getResponse().clear();
	}
	
	
}

Server::ServerException::ServerException(void)
{
	_error = "Undefined Server Exception";
}

Server::ServerException::ServerException(std::string function, std::string error)
{
	_error = function + ": " + error;
}

const char *Server::ServerException::what(void) const throw()
{
	return (this->_error.c_str());
}