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

/*
*	Will have to move to its own file.
*/
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
	server_location();
	server_location(const server_location& other);
};

std::vector<std::string> splitIntoVector(std::string str, const std::string& sep);
bool		isPort(std::string p);
int			FileExists(std::string file);
std::string ExtractFile(std::string file);
const server_location *locationByUri(const std::string& uri, const std::vector<server_location>& locs);

#endif