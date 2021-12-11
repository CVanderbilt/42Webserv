#include "Client.hpp"

Client::Client() : 
	_fd(-1),
	_status(-1),
	_response_sent(0),
	_response_left(0),
	_request(-1),
	_stat_msg(StatusMessages()),
	_time_check(ft_now())
{
}

Client::Client(int fd) : 
	_fd(fd),
	_status(-1),
	_response_sent(0),
	_response_left(0),
	_request(-1),
	_stat_msg(StatusMessages()),
	_time_check(ft_now())
{
}

Client::Client(Client const &copy) :

	_fd(copy._fd),
	_status(copy._status),
	_response_sent(copy._response_sent),
	_response_left(copy._response_left),
	_request(copy._request),
	_stat_msg(copy._stat_msg),
	_time_check(copy._time_check),
	_s(copy._s)
{
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
	_time_check = ft_now();
	if ((temp = _request.parse_chunk(chunk, bytes)) == Http_req::PARSE_ERROR)
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

int		Client::ResponseStatus(const server_location *s)
{
	/*TODO: completar con más codigos, como el 404...*/
	if (!s)
		return (_response_status = 404);
	if (_status == 0)
	{
		if (_request.protocol.compare("HTTP/1.1") != 0)
			return (_response_status = 505);
		if (_request.body.length() > _s->max_body_size)
			return (_response_status = 413);
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
	
	const server_location *s = locationByUri(_request.uri, _s->locations);
	ResponseStatus(s);
	if (_response_status < 400)
	{
		if (s->path != _request.uri.substr(0, _request.uri.find_last_of('/') + 1))
		{
			_request.uri += "/";
			_request.file_uri = "";
		}
		if (isCGI(s))
			body = ExecuteCGI(s);
		else if (_request.method.compare("GET") == 0)
			body = BuildGet(s);
		else if (_request.method.compare("POST") == 0)
			body = BuildPost(s);
		else if (_request.method.compare("DELETE") == 0)
			body = BuildDelete(s);
	}
	if (_response_status >= 400)
	{
		std::cout << "attempt to respond with error page" << std::endl;
		try
		{
			if (_s->error_pages.count(_response_status) > 0)
			{
				body = "\r\n" + ExtractFile(_s->error_pages.find(_response_status)->second);
			}
			else
			{
				std::cout << "error: " << _response_status << ", doesnt have html page" << std::endl;
				body = "\r\n";
			}
		}
		catch(const std::exception& e)
		{
			std::cout << "file not found" << std::endl;
			body = "";
			std::cerr << e.what() << '\n';
		}
	}
	stream << WrapHeader(body, s);
	_response = stream.str();
	_response_left = _response.length();
}

template<typename T>
static void AddIfNotSet(std::string& headers, const std::string& header, const T& value)
{
	std::stringstream	stream;
	stream << header << ":";
	size_t pos = headers.find(stream.str());
	if (pos == headers.npos)
	{
		std::cout << "adding " << header << ": " << value << std::endl;
		stream << " " << value << "\r\n";
		headers += stream.str();
	}
}

std::string Client::WrapHeader(const std::string& msg, const server_location *s)
{
	std::stringstream	stream;
	stream << "HTTP/1.1 " << _response_status << " " << _stat_msg[_response_status];
	std::string headers = "";
	std::string body = "";
	size_t pos = msg.find("\r\n\r\n");
	size_t pos2 = msg.find("\r\n");
	pos = pos2 >= pos ? pos : pos2;

	if (pos != msg.npos)
	{
		headers = msg.substr(0, pos + 2);
		std::cout << "headers >" << headers << "<" << std::endl;
		body = msg.substr(pos + 2, msg.npos);
	}
	AddIfNotSet(headers, "Content-type", setContentType());
	AddIfNotSet(headers, "Content-Length", body.length());
	AddIfNotSet(headers, "Date", getActualDate());
	if (_request.method.compare("GET") == 0 && s != 0)
		AddIfNotSet(headers, "Last-Modified", lastModified(s));
	if (_response_status == 301)
		AddIfNotSet(headers ,"Location", _redirect);
	if (_response_status == 301)
		AddIfNotSet(headers ,"Retry-After", 0);
	else if (_response_status == 503)
		AddIfNotSet(headers ,"Retry-After", 120);
	AddIfNotSet(headers, "Server", "Webserv/0.9");
	stream << headers << "\r\n" << body;
	return (stream.str());
}
/*
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
}*/

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

std::string Client::ExecuteCGI(const server_location *s)
{
	try
	{
		if (_request.method.compare("DELETE") == 0)
			throw 405 ;
		std::string ret;
		CGI cgi(&_request, s, _s);
		ret = cgi.executeCGI();
		size_t pos = ret.find("\r\n\r\n");
		std::string headers = ret.substr(0, pos);
		ret = ret.substr(pos + 1, ret.size() - pos - 1);
		pos = headers.find("Status");
		if (pos == headers.npos)
			pos = headers.find("status");
		if (pos != headers.npos)
		{
			headers = headers.substr(pos + 1);
			pos = headers.find(':');
			if (pos == headers.npos)
				_response_status = 500;
			else
			{
				headers = headers.substr(pos + 1, headers.npos);
				pos = headers.find_first_not_of(' ');
				headers = headers.substr(pos);
				_response_status = std::atoi(headers.c_str());
			}
		}
		return ("\r\n" + ret);
	}
	catch(int err)
	{
		_response_status = err >= 400 ? err : 500;
		return ("");
	}
	
}

std::string	Client::BuildGet(const server_location *s)
{
	_response_status = 200;
	
//	std::cout << "s->redirect = " << s->redirect << std::endl;
	if (s->redirect != "")
	{
		_response_status = 301;
		_redirect = s->redirect;
		return ("\r\n");
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
					return ("\r\n" + ExtractFile(s->root + *it));
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
				return ("\r\n" + GetAutoIndex(s->root, s->path));
			}
			_response_status = 404;
			return ("");
		}
		else
		{
			try
			{
				return ("\r\n" + ExtractFile(s->root + _request.file_uri));
			}
			catch(const std::exception& e)
			{
				_response_status = 404;
				return ("");
			}
		}
	}
	std::cout << "IT SHOULD NEVER GET HERE" << std::endl;
	_response_status = 500;
	return ("");
}

std::string	Client::BuildPost(const server_location *s)
{
	_response_status = 200;
	if (s->write_enabled)
	{
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
				}
			}
	}
	else
		_response_status = 405;
	std::cout << "========================================" << std::endl;
	if (_response_status == 200)
		return ("\r\n<html>\n<body>\n<h1>File uploaded correctly</h1>\n</body>\n</html>");
	return ("\r\n");
}

std::string	Client::BuildDelete(const server_location *s)
{
	std::string	ret;
	_req_file = s->write_path + _request.uri;
	_response_status = 200;
	if (s->write_enabled)
	{
		if (unlink(_req_file.c_str()) < 0)
		{
			_response_status = 500;
			if (errno == EACCES || errno == EPERM || errno == EROFS)
				_response_status = 403;
			return ("");
		}

		ret = "<html>\n<body>\n<h1>File deleted.</h1>\n</body>\n</html>";
		return ("\r\n" + ret);
	}
	_response_status = 403;
	return ("");
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

void		Client::setServer(server_info *s)
{
	_s = s;
	_request.max_size = s->max_body_size;
}

const Http_req&	Client::GetRequest()
{
	return (_request);
}

void Client::reset()
{
	_request.initialize(_s->max_body_size);
	_response_sent = 0;
	_response_left = 0;
	_status = -1;
}

bool Client::isCGI(const server_location *s)
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

bool	Client::hasTimedOut()
{
	if (ft_now() - TIMEOUT >= _time_check)
		return (true);
	return (false);
}

std::string	Client::lastModified(const server_location *s)
{
	char			buffer[30];
	struct stat		stats;
	struct tm		*gm;
	size_t			written;
	std::string		path;

//	std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< Last Modified >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;

	//const server_location *s = locationByUri(_request.uri, _s->locations);
	if (s)
	{
//		std::cout << "_request.uri = " << _request.uri << std::endl;
//		std::cout << "root = " << s->root << std::endl;
		if (s->index.size() > 0)
		{
			for (size_t i = 0; i < s->index.size(); i++)
			{
				path = s->root + s->index[i];
//		std::cout << "path = " << path << std::endl;
//		std::cout << "exists = " << fileExists(path) << std::endl;
				if (fileExists(path))
					break;
			}
		}
		else if (_request.file_uri != "")
			path = s->root + "/" + _request.file_uri;
//		std::cout << "path = " << path << std::endl;
		if (stat(path.c_str(), &stats) == 0)
		{
			gm = gmtime(&stats.st_mtime);
			if (gm)
			{
				written = strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gm);
				if (written <= 0)
					perror("strftime");
			}
		}
	}
	return (std::string(buffer));
}

std::string		Client::setContentType()
{
	std::string type;
	std::string str;
	size_t		i;
	
	str = _request.uri;
//	std::cout << "<<<<<<<<<<<<<<<<prueba de content types>>>>>>>>>>>>>>>>" << std::endl;
//	std::cout << "uri = " << str << std::endl;
	if ((i = str.find_last_of(".")) != str.npos)
		str = str.substr(i + 1, str.length() - i);
	std::cout << "extension = " << str << std::endl;
	if (str == "css")
		type = "text/css";
	else if (str == "js")
		type = "application/javascript";
	else if (str == "jpeg" || str == "jpg")
		type = "image/jpeg";
	else if (str == "png")
		type = "image/png";
	else if (str == "bmp")
		type = "image/bmp";
	else
		type = "text/html";
//	std::cout << "type = " << type << std::endl;
	return (type);
}