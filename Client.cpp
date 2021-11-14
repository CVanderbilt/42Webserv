#include "Client.hpp"

#include <sys/types.h>
#include <dirent.h>

Client::Client() : 
	_status(-1),
	_fd(-1),
	_response_sent(0),
	_response_left(0),
	_max_body_size(1000000),
	_stat_msg(StatusMessages())
{}

Client::Client(int fd) : 
	_status(-1),
	_fd(fd),
	_response_sent(0),
	_response_left(0),
	_max_body_size(1000000),
	_stat_msg(StatusMessages())
{}

Client::Client(Client const &copy) :

	_status(copy._status),
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
	/*TODO: completar con más codigos, como el 404...*/
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
		return (_response_status = 501);
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
	std::stringstream	stream;
	std::string			body;
	ResponseStatus();
	if (_request.method.compare("GET") == 0)
		body = BuildGet();
	else if (_request.method.compare("POST") == 0)
		BuildPost();
	else if (_request.method.compare("DELETE") == 0)
		body = BuildDelete();
	stream << BuildHeader(body.length());
	stream << body;
	_response = stream.str();
	_response_left = _response.length();
}


std::string	Client::BuildHeader(size_t size)
{
	std::stringstream	stream;
	stream << "HTTP/1.1 " << _response_status << " " << _stat_msg[_response_status] << "\r\n";
	stream << "Content-Type: text/html" << "\r\n";
	if (size > 0)
		stream << "Content-Length: " << size << "\r\n";
	stream << "\r\n";
	return (stream.str());
}

std::string	Client::BuildHeader()
{
	std::stringstream	stream;
	stream << "HTTP/1.1 " << _response_status << " " << _stat_msg[_response_status] << "\r\n";
	stream << "Content-Type: text/html" << "\r\n";
	stream << "\r\n";
	return (stream.str());
}

std::string Client::GetAutoIndex(const std::string& directory, const std::string& url_location)
{
	std::string ret = "<!DOCTYPE html>\n";
	ret += "<html>\n";
	ret += "<head>\n";
	ret += "<title>Index of" + directory + "</title>\n";
	ret += "</head>\n";
	ret += "<body>\n";
	ret += "<p>\n";

	DIR *d;
	dirent *sd;

	ret += "<h1>Index of" + directory + "</h1>";
	d = opendir(directory.c_str());
	ret += "<ul>";
	while (1)
	{
		sd = readdir(d);
		if (!sd)
			break ;
		ret += "<li><a href=\"" + url_location + sd->d_name + "\">" + sd->d_name + "</a></li>\n";
	}
	ret += "</ul>";
	ret += "</p>\n";
	ret += "</body>\n";
	ret += "</html>\n";
	return (ret);
}

std::string	Client::BuildGet()
{
	std::string	ret;

	//std::cout << "building get, (uri): >" << _request.uri << "<" << std::endl;
	//check if is asking for file(including cgi) or an index
	//	is an index if uri equals path of location
	const server_location *s = locationByUri(_request.uri, *this->_s);
	std::cout << "uri: >" << _request.uri << "<, server_location: " << s << std::endl;
	if (s)
	{
		/*
		*	locationByUri not working properly
		*/
		std::cout << "autoindex: " << s->autoindex << std::endl;
		std::cout << "path: " << s->path << std::endl;
		std::cout << "root: " << s->root << std::endl;
	}

	std::string file_in_uri = _request.uri.substr(_request.uri.find_last_of('/') + 1, _request.uri.npos);
	std::string file_path = s->root;
	if (file_path[file_path.length() - 1] != '/')
		file_path += "/";
	if (file_in_uri == "") //index
	{
		//search for an index, if found one write it on the response
		//if not found but autoindex on create autoindex
		//else 404
		for (std::vector<std::string>::const_iterator it = s->index.begin(); it != s->index.end(); it++)
		{
			try
			{
				return (ExtractFile(file_path + *it));
			}
			catch(const std::exception& e)
			{
				std::cerr << file_path + *it << " not found" << std::endl;
			}
			if (s->autoindex)
				return (GetAutoIndex(file_path, s->path));
		}
	}
	else
	{
		file_path += file_in_uri;
		try
		{
			return (ExtractFile(file_path));
		}
		catch(const std::exception& e)
		{
			std::cout << "file not found, return error page if available" << std::endl;
		}
		
	}
	return (ret);
}

void	Client::BuildPost()
{
	if (_is_CGI)
//		ExecuteCGI();
;/*TODO: build function to execute CGI*/
	else
	{
		std::ofstream file(_req_file);

		if (file.good())
		{
			file << _request.body << std::endl;
			file.close();
		}
		else
		{
			_response_status = 500;
			std::cout << "server: internal error" << std::endl;
		}
	}
}

std::string	Client::BuildDelete()
{
	std::string	ret;

	unlink(_req_file.c_str());
	ret = "<html>\n<body>\n<h1>File deleted.</h1>\n</body>\n</html>";
	return (ret);
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
		map[408] = "Request Time-out";
		map[411] = "Length Required";
		map[413] = "Payload Too Large";
		map[500] = "Internal Server Error";
		map[501] = "Not Implemented";
		map[503] = "Service Unavailable";
		map[505] = "HTTP Version Not Supported";
		return (map);
}

void		Client::setServer(std::vector<server_location> *s)
{
	_s = s;
}