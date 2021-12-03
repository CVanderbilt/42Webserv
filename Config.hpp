#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <map>
# include <vector>
# include <fstream>

class ConfigException : public std::exception
{
	private:
		std::string		_error;
	
	public:
		ConfigException(void);
		ConfigException(std::string line, std::string error);
		virtual				~ConfigException(void) throw() {};
		virtual const char* what() const throw();
};

struct location_config
{
	std::map<std::string, std::string>opts;
	std::string path;

	void parse_config(std::ifstream& file, std::string& ln);

	location_config();
	location_config(const location_config& other);
};

struct server_config
{
	std::map<std::string, std::string>	opts;
	std::vector<location_config>		loc;

	void parse_config(std::ifstream& file);

	server_config();
	server_config(const server_config& other);
};

std::vector<server_config> check_config(std::string config_file);
//std::map<std::string, std::string>	cgi_exec_path;
#endif