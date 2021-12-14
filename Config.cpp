#include "Config.hpp"
#include <iostream>

ConfigException::ConfigException()
{
	_error = "Undefined Config Exception";
}

ConfigException::ConfigException(std::string line, std::string error)
{
	_error = error + ": >" + line + "<";
}

const char *ConfigException::what(void) const throw()
{
	return (this->_error.c_str());
}

std::string TrimLine(std::string line)
{
	std::string ret;

	size_t p = line.find('#', 0);
	if (p != line.npos)
		line = line.substr(0, p);
	size_t start = 0;
	for (size_t i = 0; ; i++)
		if (i == line.size() || (line[i] != ' ' && line[i] != 9))
		{
			start = i;
			break ;
		}
	size_t end = line.size();
	for (size_t i = end - 1; i >= 0; i--)
		if (i == line.size() || (line[i] != ' ' && line[i] != 9))
		{
			end = i + 1;
			break ;
		}
	ret = line.substr(0, end);
	if (ret.length() == 0)
		return (ret);
	ret = ret.substr(start, ret.npos);
	return (ret);
}

/*
*	This function parses a location block,
*	at first a path is extracted from the first in a line >location /users/data< "users/data"
*	would be extracted.
*	On each line:
*	if a closing key >}< means end of the block, another end means an error ocurred. (block not closed).
*	else an opening keu >{< means an error was made, a location block does not have blocks inside it.
*	else if the line is >key some more values/words< another oprtion "key": "somo more values/words" is added.
*	else bad format line error
*/
void location_config::parse_config(std::ifstream& file, std::string& ln)
{
	std::string str;
	size_t p = ln.find(' ');

	if (p == ln.npos)
		throw ConfigException(ln, "missing two spaces(separators)");
	str = ln.substr(p + 1, ln.npos);
	p = str.find(' ');
	if (p == ln.npos)
		throw ConfigException(ln, "missing one space(separator)");
	path = str.substr(0, p);
	while (std::getline(file, str))
	{
		str = TrimLine(str);
		if (str.length() == 0)
			continue ;
		if (str == "}")
			return ;
		if (str.find("{") != str.npos)
			throw ConfigException(str, "cant open new block inside location");
		if (str.find(' ') != str.npos && str.find(' ') + 1 < str.size())
			this->opts[str.substr(0, str.find(' '))] = str.substr(str.find(' ') + 1, str.npos);
		else
			throw ConfigException(str, "bad format line, maybe missing value");
	}
	throw ConfigException("EOF", "missing closing key (location block)");
}

void server_config::parse_config(std::ifstream& file)
{
	std::string str;

	while (std::getline(file, str))
	{
		str = TrimLine(str);
		if (str.length() == 0)
			continue ;
		if (str == "}")
			return ;
		if (str.find("location") != str.npos)
		{
			loc.resize(loc.size() + 1);
			loc[loc.size() - 1].parse_config(file, str);
		}
		else if (str.find(' ') != str.npos && str.find(' ') + 1 < str.size())
			this->opts[str.substr(0, str.find(' '))] = str.substr(str.find(' ') + 1, str.npos);
		else
			throw ConfigException(str, "bad format line, maybe missing value");
	}
	throw ConfigException("EOF", "missing closing key (server block)");
}

std::vector<server_config> check_config(std::string config_file, std::map<std::string, std::string>	&cgi_exec_path)
{
	std::ifstream file(config_file.c_str());
	std::string str;
	std::vector<server_config> ret;

	if (!file.is_open())
	{
		perror("opening file");
		throw std::exception();
	}

	while (std::getline(file, str))
	{
		str = TrimLine(str);
		if (str.length() == 0)
			continue ;
		if (str == "server {")
		{
			ret.resize(ret.size() + 1);
			ret[ret.size() - 1].parse_config(file);
		}
		else if (str[0] == '.')
			cgi_exec_path[str.substr(0, str.find(' '))] = str.substr(str.find(' ') + 1, str.npos);
		else
			throw ConfigException(str, "server or err_pages block expected");
	}
	return (ret);
}

location_config::location_config()
{}

location_config::location_config(const location_config& other)
{
	opts = other.opts;
	path = other.path;
}

server_config::server_config()
{}

server_config::server_config(const server_config& other)
{
	opts = other.opts;
	loc = other.loc;
}