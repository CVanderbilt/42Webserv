#include "Client.hpp"

Client::Client() : 
	_fd(-1),
	_status(-1),
	_response_sent(0),
	_response_left(0),
	_max_body_size(1000000),
	_stat_msg(StatusMessages()),
	_is_CGI(false)
{
	_error_pages = NULL;
}

Client::Client(int fd) : 
	_fd(fd),
	_status(-1),
	_response_sent(0),
	_response_left(0),
	_max_body_size(1000000),
	_stat_msg(StatusMessages()),
	_is_CGI(false)
{
	_error_pages = NULL;
}

Client::Client(Client const &copy) :

	_fd(copy._fd),
	_status(copy._status),
	_response_sent(copy._response_sent),
	_response_left(copy._response_left),
	_max_body_size(copy._max_body_size),
	_stat_msg(copy._stat_msg),
	_is_CGI(copy._is_CGI)
{
	_error_pages = NULL;
} 

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

void	Client::getParseChunk(char *chunk, size_t bytes)
{
	Http_req::parsing_status temp;

//	std::cout << "parsing new chunck of size: " << bytes << std::endl;
	if ((temp = _request.parse_chunk(chunk, bytes)) == Http_req::PARSE_ERROR)
		_status = 0;
	else if (temp == Http_req::PARSE_END)
	{
		_status = 1;
		_is_CGI = isCGI();
//		std::cout << "isCGI = " << _is_CGI << std::endl;
	}
//	std::cout << _request << std::endl;
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
	if (MethodAllowed() == false)
		return (_response_status = 501);
	return (_response_status = 200);
}

bool	Client::MethodAllowed()
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
	if (_request.status == Http_req::PARSE_ERROR)
		_response_status = 400;
	else if (_request.method.compare("GET") == 0)
		body = BuildGet();
	else if (_request.method.compare("POST") == 0)
		body = BuildPost();
	else if (_request.method.compare("DELETE") == 0)
		body = BuildDelete();
	if (_response_status >= 400)
	{
		std::cout << "attempt to respond with error page" << std::endl;
		try
		{
			if (_error_pages->count(_response_status) > 0)
			{
				body = ExtractFile(_error_pages->find(_response_status)->second);
			}
			else
			{
				std::cout << "error: " << _response_status << ", doesnt have html page" << std::endl;
				body = "";
			}
		}
		catch(const std::exception& e)
		{
			std::cout << "file not found" << std::endl;
			body = "";
			std::cerr << e.what() << '\n';
		}
	}
	stream << BuildHeader(body.length());
	stream << body;
	_response = stream.str();
	std::cout << "_response = " << _response << std::endl;
	_response_left = _response.length();
}


std::string	Client::BuildHeader(size_t size)
{
	std::stringstream	stream;
	stream << "HTTP/1.1 " << _response_status << " " << _stat_msg[_response_status] << "\r\n";
	if (_response_status == 301)
	{
		stream << "Location: " <<  _redirect << "\r\n";
		stream << "Content-Length: " << 0 << "\r\n";
		stream << "\r\n";
	}
	else
	{
		stream << "Content-Type: text/html" << "\r\n";
		stream << "Content-Length: " << size << "\r\n";
		stream << "\r\n";
	}
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
		std::string name = sd->d_name;
		if (sd->d_type == DT_DIR)
			name += "/";
		else if (sd->d_type != DT_REG)
			continue ;
		ret += "<li><a href=\"" + url_location + name + "\">" + name + "</a></li>\n";
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

	_response_status = 200;
	const server_location *s = locationByUri(_request.uri, *this->_s);
	if (!s)
	{
		_response_status = 404;
		return ("");
	}
	std::cout << "s->redirect = " << s->redirect << std::endl;
	if (_is_CGI)
	{
		CGI cgi(_request, s);
		_response_cgi = cgi.executeCGI();
		size_t pos = _response_cgi.find("\r\n\r\n");
		if (pos != _response_cgi.npos)
			ret = _response_cgi.substr(pos + 4, _response_cgi.size() - pos - 4);
		return (ret);
	}
	else if (s->redirect != "")
	{
		_response_status = 301;
		_redirect = s->redirect;
		return ("");
	}
	else 
	{
		if (_request.file_uri == "") //index
		{
			for (std::vector<std::string>::const_iterator it = s->index.begin(); it != s->index.end(); it++)
			{
				std::cout << "  -trying index: >" << s->root << *it << "<" << std::endl;
				try
				{
					return (ExtractFile(s->root + *it));
				}
				catch(const std::exception& e)
				{
					std::cerr << s->root + *it << " not found" << std::endl;
				}
			}
			std::cout << "Not index found" << std::endl;
			if (s->autoindex)
			{
				std::cout << "Sending autoindex" << std::endl;
				return (GetAutoIndex(s->root, s->path));
			}
			_response_status = 404;
			return ("");
		}
		else
		{
			try
			{
				return (ExtractFile(s->root + _request.file_uri));
			}
			catch(const std::exception& e)
			{
				_response_status = 404;
				return ("");
			}
		}
	}
	std::cout << "IT SHOULD NEVER GET HERE" << std::endl;
	return (ret);
}

std::string	Client::BuildPost()
{
	std::string ret = "";
	const server_location *s = locationByUri(_request.uri, *this->_s);

	_response_status = 200;
	if (!s)
	{
		_response_status = 404;
		return (ret);
	}
	if (_is_CGI)
	{
		CGI cgi(_request, s);
		_response_cgi = cgi.executeCGI();
		size_t pos = _response_cgi.find("\r\n\r\n");
		if (pos != _response_cgi.npos)
			ret = _response_cgi.substr(pos + 4, _response_cgi.size() - pos - 4);
	}
	else
	{
		if (s->write_enabled)
			for (size_t i = 0; i < _request.mult_form_data.size(); i++)
				if (_request.mult_form_data[i].filename != "")
				{
					std::ofstream file;
					_req_file = s->write_path + "/" + _request.mult_form_data[i].filename;
					file.open(_req_file.c_str());
					if (file.is_open() && file.good())
					{
						file.write(_request.mult_form_data[i].body.c_str(), _request.mult_form_data[i].body.length());
						file.close();
					}
					else
					{
						_response_status = 500;
						return (ret);
					}
				}
	}
	std::cout << "========================================" << std::endl;
	return (ret);
}

std::string	Client::BuildDelete()
{
	std::string	ret;
	const server_location *s = locationByUri(_request.uri, *this->_s);

	_req_file = s->write_path + _request.uri;

	_response_status = 200;
	if (unlink(_req_file.c_str()) < 0)
	{
		_response_status = 500;
		if (errno == EACCES || errno == EPERM || errno == EROFS)
			_response_status = 403;
		return ("");
	}

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

void		Client::setServer(std::vector<server_location> *s, std::map<int, std::string> *epages)
{
	_s = s;
	_error_pages = epages;
}

void		Client::setServer(std::vector<server_location> *s)
{
	_s = s;
}

const Http_req&	Client::GetRequest()
{
	return (_request);
}

void Client::reset()
{
	_request.initialize(this->_max_body_size);
	_response_sent = 0;
	_response_left = 0;
}


bool Client::isCGI()
{
	std::string	str;
	size_t		i;
	
	if ((i = _request.uri.find("?")) != _request.uri.npos)
		str = _request.uri.substr(0, i);
	else
		str = _request.uri;
	if ((i = str.find_last_of(".")) != str.npos)
		str = str.substr(i, str.length() - i);
//std::cout << "str = " << str << std::endl;
	const server_location *s = locationByUri(_request.uri, *this->_s);
	if(!s)
		return false;
	for (std::vector<std::string>::const_iterator it = s->cgi.begin(); it != s->cgi.end(); it++)
	{
//std::cout << "cgis = " << *it << std::endl;
		if (str == *it)
			return true;
	}
	return false;
}