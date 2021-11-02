#include "Server.hpp"

Server::Server() :
	_addrlen(sizeof(_addr)), 
	_pfds(new pollfd[MAX_CONNEC])
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
	_addr.sin_port = htons(PORT);
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

		_pfds[_fd_count].fd = new_fd;
		_pfds[_fd_count].events = POLLIN | POLLOUT;
		_fd_count++;
		_clients[new_fd] = new_client;
		std::cout << "pollserver: new connection on socket " << new_fd << std::endl;
	}
	else
	{
		std::cout << "Conexion no aceptada" << std::endl;
		/*TODO: send response to the client rejecting connection */
		close(new_fd);
	}
}

void	Server::read_message(int i)
{
	char	*buffer = new char[BUFFER_SIZE + 1];
	int 	numbytes;
	
	std::cout << "leyendo del fd = " << _pfds[i].fd << std::endl;
	if ((numbytes = recv(_pfds[i].fd, buffer, BUFFER_SIZE, 0)) < 0)
	{
		close(_pfds[i].fd);
		_pfds[i].fd = -1;
		/*TODO: del from pfds struct array*/
		return ;
	}
	else if (numbytes == 0)
	{
		std::cout << "El cliente cerró la conexion" << std::endl;
	}

	buffer[numbytes] = '\0';
	if (_clients.count(_pfds[i].fd))
		_clients[_pfds[i].fd].getParseChunk(buffer);
	std::cout << "pollserver: message read and parsed on socket " << _pfds[i].fd << std::endl;
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
			std::cout << "estamos en POLLIN en i = " << i << std::endl;
			if (_pfds[i].fd == _server_fd)
				accept_connection();
			else
				read_message(i);
		}		
		else if(_pfds[i].revents & POLLOUT)
		{
			std::cout << "estamos en POLLOUT en i = " << i << std::endl;
			int status;
			if (_clients.count(_pfds[i].fd))
				status = _clients[_pfds[i].fd].getStatus();
			if (status > 0)
			{
				/*TODO: prepare response message*/
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

				send(_pfds[i].fd, response.c_str(), response.length(), 0);
				std::cout << "Mensaje enviado" << std::endl;
			}
			else if (status == 0)
				std::cout << "Parse error" << std::endl;
			else
				continue;
			close(_pfds[i].fd);
			_clients.erase(_pfds[i].fd);
			_pfds[i].fd = -1; /*TODO: MEJORAR*/
		}
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