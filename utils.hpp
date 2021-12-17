#ifndef UTILS_HPP
# define UTILS_HPP

# include <vector>
# include <map>
# include <string>
# include <sys/stat.h>
# include <fstream>
# include <sstream>
# include "Config.hpp"
# include <cstdlib>
# include <stdint.h>

# define PRINT_MODE
struct server_location 
{
	std::string							root; //is the path to the files that are mapped to the uri of the browser
	std::string							path; //is the uri in the browser
	bool								autoindex;
	std::vector<std::string>			cgi;
	std::vector<std::string>			index;
	bool								write_enabled;
	std::string							write_path;
	std::string							server_name;
	std::string							port;
	std::string							redirect;
	bool								allow_get;
	bool								allow_post;
	bool								allow_delete;
	server_location();
	server_location(const server_location& other);
};

struct	server_info
{
		std::vector<std::string>			names;
		std::map<int, std::string>			error_pages;
		std::vector<server_location>		locations;
		std::string							port;
		std::map<std::string,std::string>	const *cgi_paths;
		size_t								max_body_size;
		//client size, no se si algo más, sobre la marcha
		server_info();
		server_info(const server_info& other);
};

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep);
bool		isPort(std::string p);
bool		fileExists(std::string file);
std::string ExtractFile(std::string file);
uint64_t 	ft_now(void);
std::string getActualDate(void);
bool		isDirectory(const char *path);

#endif