#include "Server.hpp"
#include "utils.hpp"
/*
Server::Server() :
	_fd_size(2),
	_port(8080),
	_addrlen(sizeof(_addr)),
	_pfds(new pollfd[_fd_size])
{}

Server::Server(int port) :
	_fd_size(2),
	_port(port),
	_addrlen(sizeof(_addr)),
	_pfds(new pollfd[_fd_size])
{}*/

Server::Server(server_config const& s, std::map<std::string, std::string>	*cgi_exec_path) :
	_fd_size(2),
	_addrlen(sizeof(_addr)),
	_pfds(new pollfd[_fd_size])
{
	//_cgi_paths->count(".py");
	addServer(s);
	_configuration.cgi_paths = cgi_exec_path;
}

void Server::addServer(server_config const& s)
{
	for (std::map<std::string, std::string>::const_iterator it = s.opts.begin(); it != s.opts.end(); it++)
	{
		if (it->first == "port" && isPort(it->second))
			//_port = std::atoi(it->second.c_str());
			//_configuration.port = std::atoi(it->second.c_str());
			_configuration.port = it->second;
		else if (it->first == "server_name")
			_configuration.names = splitIntoVector(it->second, " ");
		else if (it->first.length() == 3 && (it->first[0] == '4' || it->first[0] == '5') && 
		isdigit(it->first[1]) && isdigit(it->first[2]))
		{
			std::vector<std::string> aux = splitIntoVector(it->second, " ");
			if (aux.size() != 1)
				throw ServerException("Configuration", "Invalid value in server block: >" + it->first + "<");
			_configuration.error_pages[std::atoi(it->first.c_str())] = aux[0];
		}
		else if (it->first == "body_size")
			_configuration.max_body_size = std::atoll(it->second.c_str());
		else
			throw ServerException("Configuration", "Invalid key in server block: >" + it->first + "<");
	}

	int idx = -1;
	for (std::vector<location_config>::const_iterator it = s.loc.begin(); it != s.loc.end(); ++it)
	{
		idx++;
		_configuration.locations.resize(_configuration.locations.size() + 1);
		_configuration.locations[idx].path = it->path[it->path.length() - 1] == '/' ? it->path : it->path + "/";
		for (std::map<std::string, std::string>::const_iterator lit = it->opts.begin(); lit != it->opts.end(); lit++)
		{
			if (lit->first == "root")
				_configuration.locations[idx].root = lit->second[lit->second.length() - 1] == '/' ? lit->second : lit->second + "/";
			else if (lit->first == "autoindex")
			{
				if (lit->second == "on")
					_configuration.locations[idx].autoindex = true;
				else if (lit->second != "off")
					throw ServerException("Configuration", "Invalid value of autoindex field");
			}
			else if (lit->first == "cgi")
				_configuration.locations[idx].cgi = splitIntoVector(lit->second, " ");
			else if (lit->first == "index")
				_configuration.locations[idx].index = splitIntoVector(lit->second, " ");
			else if (lit->first == "write_enabled")
			{
				_configuration.locations[idx].write_enabled = true;
				_configuration.locations[idx].write_path = lit->second;
			}
			else if (lit->first == "redirection")
				_configuration.locations[idx].redirect = lit->second;
			else
				throw ServerException("Configuration", "Invalid key in location block: >" + lit->first + "<");
		}
	}
}

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
	_addr.sin_port = htons(std::atoi(_configuration.port.c_str()));
	_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(_addr.sin_zero, '\0', sizeof(_addr.sin_zero));
	int optval = 1;
	if ((setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int))) == -1)
	{
		close(_server_fd);
		throw ServerException("In setsockopt", "failed for some reason with option SO_REUSEADDR");
	}
#ifndef __linux__
	if ((setsockopt(_server_fd, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(int))) == -1)
	{
		close(_server_fd);
		throw ServerException("In setsockopt", "failed for some reason with option SO_NOSIGPIPE");
	}
#endif
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
	_pfds[0].events = POLLIN;
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
		new_client.setServer(&_configuration);
		add_to_pfds(new_fd);
		if (_clients.count(new_fd) > 0)
			std::cout << "=====================================================" << std::endl;
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
	long int 	numbytes;
	
	if ((numbytes = recv(_pfds[i].fd, buffer, BUFFER_SIZE, 0)) < 0)
	{
		close_fd_del_client(i); //todo  msg
		perror("recv");
		std::cout << "server: recv error---------------------------------------------------------------------" << std::endl;
	}
	else if (numbytes == 0)
	{
		std::cout << "server: client " << _pfds[i].fd << " closed connection" << std::endl;
		close_fd_del_client(i);
	}
	else
	{
		buffer[numbytes] = '\0'; //falso ?
		if (_clients.count(_pfds[i].fd))
			_clients[_pfds[i].fd].getParseChunk(buffer, numbytes);
		std::cout << "server: message read and parsed on socket " << _pfds[i].fd << std::endl;
	}
	delete[] buffer;
}

void	Server::server_listen()
{
	int poll_count = poll(_pfds, _fd_count, 1);
	if (poll_count == -1)
	{
		close(_server_fd);
		throw ServerException("In poll", "failed for some reason");
	}
	for (size_t i = 0; i < _fd_count; i++)
	{
		if (_pfds[i].fd != _server_fd && _clients.count(_pfds[i].fd) == 0)
		{
			std::cout << "i: " << i << ", fd: " << _pfds[i].fd << " not found on clients" << std::endl;
			_pfds[i].fd = -1;
		}
		else if (_pfds[i].fd != _server_fd && _clients[_pfds[i].fd].hasTimedOut())
		{
			std::cout << "fd: " << _pfds[i].fd << " its client has timed out" << std::endl;
			close_fd_del_client(i);
		}
		else if (_pfds[i].revents & POLLIN)
		{
			std::cout << ">>>>>>>>>INCOMING CONNECTION<<<<<<<<<" << std::endl;
			std::cout << "from client: " << i << ", fd: " << _pfds[i].fd << std::endl; //client 0 -> server socket
			if (_pfds[i].fd == _server_fd)
				accept_connection();
			else
				read_message(i);
			std::cout << ">>>>>>>>>INCOMING CONNECTION PROCESSED<<<<<<<<<" << std::endl << std::endl;
		}
		else if(_pfds[i].revents & POLLOUT)
		{
			if (_pfds[i].fd == _server_fd) //por si las moscas
				continue ;
			int status = 0; //revisar si es posible que entre aquí pero no pueda entrar en el if, y si se da el caso ver que hay que hacer
			if (_clients.count(_pfds[i].fd))
			{
				status = _clients[_pfds[i].fd].getStatus();
				if (status >= 0)
					send_response(i);
				else
					continue;
			}
			std::cout << "<<<<<<<<<READY TO ANSWER>>>>>>>>>" << std::endl;
			const std::map<std::string, std::string>& head_info = _clients[_pfds[i].fd].GetRequest().head;
			if (status == 0)
				close_fd_del_client(i);
			else
			{
				std::map<std::string, std::string>::const_iterator cnt = head_info.find("connection"); //en algún momento habrá que guardar todas las keys en mayusculas o en minusculas de forma consistente
				if (cnt != head_info.end() && cnt->second == "close")
				{
					std::cout << "Closing " << _pfds[i].fd << " beacuse was not keep alive" << std::endl;
					close_fd_del_client(i); //todo error msg
				}
				else
				{
					_clients[_pfds[i].fd].reset();
					std::cout << "Client " << _pfds[i].fd << " reset (keeping alive)" << std::endl;
				}
			}
			std::cout << "<<<<<<<<<ANSWERED, CLOSED OR RESETED>>>>>>>>>" << std::endl << std::endl;
		}
		if(_pfds[i].fd == -1)
		{
			del_from_pfds(i);
			i--;
		}
	}
}

void	Server::add_to_pfds(int new_fd)
{
	if (_fd_count == _fd_size - 1)
	{
		_fd_size = 2 * _fd_size > MAX_CONNEC? MAX_CONNEC : _fd_size * 2;
		pollfd	*temp = new pollfd[_fd_size];
		for (size_t i = 0; i < _fd_count; i++)
			temp[i] = _pfds[i];
		delete[] _pfds;
		_pfds = temp;		
	}
	std::cout << "created client on pos " << _fd_count << std::endl;
	_pfds[_fd_count].fd = new_fd;
	_pfds[_fd_count].events = POLLIN | POLLOUT;
	_fd_count++;
}

void	Server::del_from_pfds(int i)
{
	//
	std::cout << "i(" << i << "), count: " << _fd_count << "), fd_list:";
	for (size_t i = 0; i < _fd_count; i++)
		std::cout << " " << _pfds[i].fd;
	std::cout << std::endl;
	_pfds[i] = _pfds[_fd_count - 1];
	_fd_count--;
	std::cout << "fd_list now:";
	for (size_t i = 0; i < _fd_count; i++)
		std::cout << " " << _pfds[i].fd;
	std::cout << std::endl;
}

void	Server::close_fd_del_client(int i)
{
	std::cout << "closing client (" << i << "): " << _pfds[i].fd << std::endl;
	if (close(_pfds[i].fd) < 0)
		std::cout << "problem closing" << std::endl;
	_clients.erase(_pfds[i].fd);
	_pfds[i].fd = -1;
}

void	Server::send_response(int i)
{
	size_t val_sent;

	_clients[_pfds[i].fd].BuildResponse();
//	std::cout << "going to send(" << _pfds[i].fd << ", (str + " << _clients[_pfds[i].fd].getResponseSent() << "), " << _clients[_pfds[i].fd].getResponse().length() << ", 0" << std::endl;
	val_sent = send(_pfds[i].fd, _clients[_pfds[i].fd].getResponse().c_str() + _clients[_pfds[i].fd].getResponseSent(), _clients[_pfds[i].fd].getResponse().length(), 0);
//	std::cout << "sent succesfully" << std::endl;
	if ((val_sent < 0))
		std::cout << "server: error sending response on socket " << _pfds[i].fd << std::endl;
		//si se da este caso habría que cerrar la conexión y a otra cosa
	else if (val_sent < _clients[_pfds[i].fd].getResponseLeft())
	{
		std::cout << "server: partial response sent on socket" << std::endl;
		_clients[_pfds[i].fd].setResponseSent(_clients[_pfds[i].fd].getResponseLeft() + val_sent);
		_clients[_pfds[i].fd].setResponseLeft(_clients[_pfds[i].fd].getResponse().length() - _clients[_pfds[i].fd].getResponseSent());
	}
	else
	{
		std::cout << "server: response sent on socket " << _pfds[i].fd << std::endl << std::endl;
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
