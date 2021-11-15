#include "utils.hpp"
#include <iostream>

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep)
{
	std::vector<std::string> ret;

	while (1)
	{
		size_t pos = str.find(sep);
		
		ret.push_back(str.substr(0, pos));
		if (pos == str.npos)
			break ;
		str.erase(0, pos + 1);
	}
	return (ret);
}

bool isPort(std::string p)
{
	for (size_t i = 0; i < p.size(); i++)
		if (!std::isdigit(p[i]))
			return (false);
	int n = std::stoi(p);
	if (n < 0 || n > 65535)
		return (false);
	return (true);
}

int FileExists(std::string file)
{
	struct stat st;

	return (stat(file.c_str(), &st));
}

std::string ExtractFile(std::string filename)
{
	
	std::ostringstream contents;
    std::ifstream in(filename, std::ios::in | std::ios::binary);

    if (!in.is_open() || !in.good())
		throw std::exception();
	contents << in.rdbuf();
	in.close();
	return contents.str();
}

//	uri:	/locations/some_file
//	path:	/locations
//	
bool checkUri(const std::string& path, const std::string& uri)
{
	size_t path_len = path.length();
	size_t uri_len = uri.length();
	for (size_t i = 0; i < path_len; i++)
	{
		if (i == uri_len || path[i] != uri[i])
			return (false);
	}
	return (true);
}

const server_location *locationByUri(const std::string& uri, const std::vector<server_location>& locs)
{
	size_t pos = uri.find_last_of('/');
	if (pos == uri.npos)
		return (nullptr);
	std::string uri_directory = uri.substr(0, uri.find_last_of('/') + 1);
	std::cout << "searching for location with path: >" << uri_directory << "<" << std::endl;
	for (std::vector<server_location>::const_iterator it = locs.begin(); it != locs.end(); it++)
		if (it->path == uri_directory)
			return (&(*it));
	return (nullptr);
}

server_location::server_location():
	autoindex(false)
{}
server_location::server_location(const server_location& other):
	root(other.root),
	path(other.path),
	autoindex(other.autoindex),
	cgi(other.cgi),
	index(other.index)
{}
