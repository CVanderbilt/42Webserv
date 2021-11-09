#include "Client.hpp"

Client::Client() : 
	_status(-1),
	_is_sending(0),
	_fd(-1),
	_response_sent(0),
	_response_left(0),
	_max_body_size(1000000),
	_stat_msg(StatusMessages())
{}

Client::Client(int fd) : 
	_status(-1),
	_is_sending(0),
	_fd(fd),
	_response_sent(0),
	_response_left(0),
	_max_body_size(1000000),
	_stat_msg(StatusMessages())
{}

Client::Client(Client const &copy) :

	_status(copy._status),
	_is_sending(copy._is_sending),
	_fd(copy._fd),
	_response_sent(copy._response_sent),
	_response_left(copy._response_left),
	_max_body_size(copy._max_body_size),
	_stat_msg(copy._stat_msg)
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

int		Client::ResponseStatus()
{
	if (_status == 0)
	{
		if (_request.protocol.compare("HTTP/1.1") != 0)
			return (_response_status = 505);
		else if (_request.body.length() > _max_body_size)
			return (_response_status = 413);
		else
			return (_response_status = 400);
	}
	if (MethodAllowed(_request.method) == false)
		return (_response_status = 405);
	return (_response_status = 200);
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

void	Client::BuildResponse()
{
	std::stringstream stream;
	
	stream << "HTTP/1.1 " << ResponseStatus() << " " << _stat_msg[_response_status] << "\r\n";

//	stream << "\r\n";
//	stream << "<html><body><h1>It works!</h1></body></html>";
	_response = stream.str();
}

std::map<int, std::string>	Client::StatusMessages()
{
	std::map<int, std::string> map;

		map[200] = "OK";
		map[201] = "Created";
		map[202] = "Accepted";
		map[204] = "No Content";
		map[301] = "Moved Permanently";
		map[302] = "Moved Temporarily";
		map[400] = "Bad Request";
		map[401] = "Unauthorized";
		map[403] = "Forbidden";
		map[404] = "Not Found";
		map[405] = "Not Allowed";
		map[406] = "Not Acceptable";
		map[411] = "Length Required";
		map[413] = "Payload Too Large";
		map[500] = "Internal Server Error";
		map[501] = "Not Implemented";
		map[503] = "Service Unavailable";
		map[505] = "HTTP Version Not Supported";
		return (map);
}

