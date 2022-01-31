#include "utils.hpp"
#include <iostream>
#include <algorithm>
#include <sys/time.h>

uint64_t ft_now(void)
{
	static struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * (uint64_t)1000) + (tv.tv_usec / 1000));
}

std::string getActualDate(void)
	{
		struct timeval tv;
		char buffer[30];
		size_t written;
		struct tm *gm;

		if (gettimeofday(&tv, NULL) != 0)
		{
			perror("gettimeofday");
		}
		gm = gmtime(&tv.tv_sec);
		if (gm)
		{
			written = strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gm);
			if (written <= 0)
				perror("strftime");
		}
		return (std::string(buffer));
	}

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

bool fileExists(std::string file)
{
	struct stat st;

	return (stat(file.c_str(), &st) == 0);
}

std::string ExtractFile(std::string filename)
{
	
	std::ostringstream contents;
    std::ifstream in;

	if(fileExists(filename))
	{
		in.open(filename.c_str(), std::ios::in | std::ios::binary);
		if (!in.is_open() || !in.good())
			return ("");
		contents << in.rdbuf();
		in.close();
		return contents.str();
	}
	return ("");
}

bool isDirectory(const char *path)
{
	struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

server_location::server_location():
	autoindex(false),
	allow_get(false),
	allow_post(false),
	allow_delete(false)
{}
server_location::server_location(const server_location& other):
	root(other.root),
	path(other.path),
	autoindex(other.autoindex),
	cgi(other.cgi),
	index(other.index),
	write_enabled(other.write_enabled),
	write_path(other.write_path),
	allow_get(other.allow_get),
	allow_post(other.allow_post),
	allow_delete(other.allow_delete)
{}

server_info::server_info():
	port("8080"),
	max_body_size((size_t)-1)
{}

server_info::server_info(const server_info& other):
	names(other.names),
	error_pages(other.error_pages),
	locations(other.locations),
	port(other.port),
	cgi_paths(other.cgi_paths),
	max_body_size(other.max_body_size)
{}

std::string	toLowerString(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return (str);
}
