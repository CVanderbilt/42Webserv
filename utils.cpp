#include "utils.hpp"

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep)
{
	std::vector<std::string> ret;
	size_t start = 0;
	size_t sep_size = sep.size();

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
	for (int i = 0; i < p.size(); i++)
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
	for (int i = 0; i < path_len; i++)
	{
		if (i == uri_len || path[i] != uri[i])
			return (false);
	}
	return (true);
}

const server_location *locationByUri(const std::string& uri, const std::vector<server_location>& locs)
{
	for (int i = 0; i < locs.size(); i++)
		if (checkUri(locs[i].path, uri))
		{
			return (&locs[i]);
		}
	return (nullptr);
}

server_location::server_location():
	autoindex(false)
{}
server_location::server_location(const server_location& other):
	path(other.path),
	root(other.root),
	autoindex(other.autoindex),
	cgi(other.cgi),
	index(other.index)
{}
