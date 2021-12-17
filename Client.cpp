#include "Client.hpp"

Client::Client() : 
	_status(-1),
	_response_status(200),
	_response_sent(0),
	_response_left(0),
	_request(-1),
	_stat_msg(StatusMessages()),
	_time_check(ft_now())
{
}

Client::Client(Client const &copy) :
	_status(copy._status),
	_response_status(copy._response_status),
	_response_sent(copy._response_sent),
	_response_left(copy._response_left),
	_request(copy._request),
	_stat_msg(copy._stat_msg),
	_redirect(copy._redirect),
	_time_check(copy._time_check),
	_error_pages(copy._error_pages),
	_s(copy._s)
{
} 

int		Client::getStatus()
{
	return (_status);
}

void	Client::getParseChunk(char *chunk, size_t bytes)
{
	Http_req::parsing_status temp;

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
	if (_response_status >= 400)
		return (1);
	_response_status = 200;
	if (!s)
		return (_response_status = 404);
	if ( (_request.method == "GET" && !s->allow_get) ||
		(_request.method == "POST" && !s->allow_post) ||
		(_request.method == "DELETE" && !s->allow_delete) )
		return (_response_status = 405);
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

std::string	Client::BuildError()
{
	std::string body;
	if (_s->error_pages.count(_response_status) > 0 && fileExists(_s->error_pages[_response_status]))
	{	
		std::string str = ExtractFile(_s->error_pages.find(_response_status)->second);
		body = "\r\n" + str;
	}
	else
	{
		std::stringstream	stream;
		stream << "\r\n<html>\n<body>\n<h1>";
		stream << _response_status << " " << _stat_msg[_response_status];
		stream << "</h1>\n</body>\n</html>";
		body = stream.str();
	}
	return (body);
}

void	Client::BuildResponse()
{
	std::stringstream	stream;
	std::string			body;
	
	LPair lpair = locationByUri(_request.uri, _s->locations);
	ResponseStatus(lpair.first);
	if (_response_status < 400)
	{
		std::cout << "location with path: >" << lpair.first->path << "<" << std::endl;
		std::cout << "status ahora " << _response_status << std::endl;
		/*			REVISAR BIEN!!!1
		if (lpair.first->path != _request.uri.substr(0, _request.uri.find_last_of('/') + 1))
		{
			_request.uri += "/";
			_request.file_uri = "";
		}*/
		if (lpair.first->redirect != "")
		{
			_response_status = 301;
			_redirect = lpair.first->redirect;
			body = "\r\n";
		}
		else if (isCGI(lpair.first))
			body = ExecuteCGI(lpair.first);
		else if (_request.method.compare("GET") == 0)
			body = BuildGet(lpair);
		else if (_request.method.compare("POST") == 0)
			body = BuildPost(lpair);
		else if (_request.method.compare("DELETE") == 0)
			body = BuildDelete(lpair);
	}
	if (_response_status >= 400)
		body = BuildError();
	stream << WrapHeader(body, lpair.first);
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
		body = msg.substr(pos + 2, msg.npos);
	}
	AddIfNotSet(headers, "Content-Type", setContentType());
	AddIfNotSet(headers, "Content-Length", body.length());
	AddIfNotSet(headers, "Date", getActualDate());
	if (_request.method.compare("GET") == 0 && s != 0)
		AddIfNotSet(headers, "Last-Modified", lastModified(s));
	if (_response_status == 301)
	{
		AddIfNotSet(headers ,"Location", _redirect);
		AddIfNotSet(headers ,"Retry-After", 0);
	}
	else if (_response_status == 503)
		AddIfNotSet(headers ,"Retry-After", 120);
	AddIfNotSet(headers, "Server", "Webserv/0.9");
	stream << headers << "\r\n" << body;
	return (stream.str());
}

std::string Client::GetAutoIndex(const std::string& directory, LPair& lpair)
{
	std::string ret = "<!DOCTYPE html>";
	ret += "<html>";
	ret += "<head>";
	ret += "<title>Index of" + directory + "</title>";
	ret += "</head>";
	ret += "<body>";
	ret += "<p>";

	DIR *d;
	dirent *sd;

	ret += "<h1>Index of" + directory + "</h1>";
	d = opendir(directory.c_str());
	if (!d)
	{
		if (errno == EACCES)
			_response_status = 403;
		else if (errno == ENOENT)
			_response_status = 404;
		else
			_response_status = 500;
		return("");
	}
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
		ret += "<li><a href=\"" + lpair.first->path + lpair.second + name + "\">" + name + "</a></li>";
	}
	ret += "</ul>";
	ret += "</p>";
	ret += "</body>";
	ret += "</html>";
	return (ret);
}

void		Client::CheckCGIHeaders(std::string headers)
{
		size_t pos = headers.find("Status");
		if (pos == headers.npos)
			pos = headers.find("status");
		if (pos != headers.npos)
		{
			headers = headers.substr(pos + 1);
			pos = headers.find(':');
			if (pos == headers.npos)
				throw 500;
			else
			{
				headers = headers.substr(pos + 1, headers.npos);
				pos = headers.find_first_not_of(' ');
				headers = headers.substr(pos);
				_response_status = std::atoi(headers.c_str());
			}
		}
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
		if (pos == ret.npos)
			throw 500 ;
		CheckCGIHeaders(ret.substr(0, pos));
		return (ret);
	}
	catch(int err)
	{
		_response_status = err >= 400 ? err : 500;
		return ("");
	}
	
}

std::string	Client::GetFile(LPair& lpair)
{
	std::string ret = ExtractFile(lpair.first->root + lpair.second);
	if (ret != "")
		return ("\r\n" + ret);
	_response_status = 404;
	return (ret);
}

std::string	Client::GetIndex(LPair& lpair, std::string& directory)
{
	std::string ret;

	for (std::vector<std::string>::const_iterator it = lpair.first->index.begin(); it != lpair.first->index.end(); it++)
	{
		ret = ExtractFile(directory + *it);
		if (ret != "")
			return ("\r\n" + ret);
	}
	if (lpair.first->autoindex)
		return ("\r\n" + GetAutoIndex(directory, lpair));
	_response_status = 404;
	return ("");
}

std::string	Client::BuildGet(LPair& lpair)
{
	std::string aux = lpair.first->root + lpair.second;
	if (*(aux.end() - 1) == '/')
		return (GetIndex(lpair, aux));
	return (GetFile(lpair));
}

std::string	Client::BuildPost(LPair& lpair)
{
	if (lpair.first->write_enabled)
	{
		for (size_t i = 0; i < _request.mult_form_data.size(); i++)
			if (_request.mult_form_data[i].filename != "")
			{
				std::ofstream file;
				std::string _req_file = lpair.first->write_path + "/" + _request.mult_form_data[i].filename;
				file.open(_req_file.c_str());
				if (file.is_open() && file.good())
				{
					file.write(_request.mult_form_data[i].body.c_str(), _request.mult_form_data[i].body.length());
					file.close();
				}
				else
					_response_status = 500;
			}
	}
	else
		_response_status = 405;
	if (_response_status == 200)
		return ("\r\n<html>\n<body>\n<h1>File uploaded correctly</h1>\n</body>\n</html>");
	return ("\r\n");
}

std::string	Client::BuildDelete(LPair& lpair)
{
	std::string	ret;
	std::string _req_file = lpair.first->write_path + lpair.second;
	if (lpair.first->write_enabled)
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
	_response_status = 200;
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
	for (std::vector<std::string>::const_iterator it = s->cgi.begin(); it != s->cgi.end(); it++)
		if (str == *it)
			return true;
	return false;
}

bool	Client::hasTimedOut()
{
	if (ft_now() - TIMEOUT >= _time_check)
	{
		_response_status = 408;
		return (true);
	}
	return (false);
}

std::string	Client::lastModified(const server_location *s)
{
	char			buffer[30];
	struct stat		stats;
	struct tm		*gm;
	size_t			written;
	std::string		path;

	if (s->index.size() > 0)
	{
		for (size_t i = 0; i < s->index.size(); i++)
		{
			path = s->root + s->index[i];
			if (fileExists(path))
				break;
			path = s->root;
		}
	}
	else if (_request.file_uri != "")
		path = s->root + "/" + _request.file_uri;
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
	return (std::string(buffer));
}

std::string		Client::setContentType()
{
	std::string type;
	std::string str;
	size_t		i;
	
	str = _request.uri;
	if ((i = str.find_last_of(".")) != str.npos)
		str = str.substr(i + 1, str.length() - i);
	if (str == "css")
		type = "text/css";
	else if (str == "js")
		type = "application/javascript";
	else if (str == "jpeg" || str == "jpg")
		type = "image/jpeg";
	else if (str == "png")
		type = "image/png";
	else if (str == "pdf")
		type = "application/pdf";
	else if (str == "bmp")
		type = "image/bmp";
	else
		type = "text/html";
	return (type);
}

static size_t inPath(const std::string& path, const std::string& uri, size_t uri_len, size_t path_len)
{
	if (path_len > uri_len)
		return (0);
	for (size_t i = 0; i < path_len; i++)
		if (path[i] != uri[i])
			return (0);
	return (path_len);
}
//const server_location *Client::locationByUri(const std::string& rawuri, const std::vector<server_location>& locs)
Client::LPair Client::locationByUri(const std::string& rawuri, const std::vector<server_location>& locs)
{
	size_t uri_len = rawuri.length();
	size_t biggest_coincidence = 0;
	LPair ret;

	ret.first = NULL;
	std::cout << "rawuri: >" << rawuri << "<" << std::endl;
	for (size_t i = 0; i < locs.size(); i++)
	{
		size_t path_len = locs[i].path.length();
		std::cout << "against path: >" << locs[i].path << "<" << std::endl;
		size_t aux = inPath(locs[i].path, rawuri, uri_len, path_len);
		std::cout << "aux: " << aux << " biggest coincidence: " << biggest_coincidence << std::endl;
		if (aux > biggest_coincidence) //es un path válildo coincide hast aux
		{
			ret.first = &locs[i];
			ret.second = "";
			std::cout << "new best option" << std::endl;
			biggest_coincidence = aux;
			if (path_len == uri_len)
			{
				std::cout << "devuelve la location porq coinciden exactamente" << std::endl;
				break ;
			}
			std::cout << "uri_len: " << uri_len << std::endl;
			std::cout << "path: " << rawuri.substr(0, aux) << std::endl;
			if (aux < uri_len)
			{
				ret.second = rawuri.substr(aux, std::string::npos);
				std::cout << "rest: " << ret.second << std::endl;
			}
		}
		else
			std::cout << "discarded" << std::endl;
	}
	return (ret);
}
