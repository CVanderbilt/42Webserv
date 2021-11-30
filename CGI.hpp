#pragma once

#include "Http_req.hpp"
#include "utils.hpp"
#include "Client.hpp"
#include "Config.hpp"
#include "Server.hpp"
#include <sys/types.h>
#include <sys/wait.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define SIDE_OUT 0
#define SIDE_IN 1

class CGI
{
	private:
		std::string	_path_cgi;
		std::string	_name_cgi;
		std::string	_response_cgi;
		int			_CGI_fd;
		Http_req	_request;
		std::vector<std::string> _env_vec;

		std::vector<std::string> envToVector(char **env);
		char	**vectorToEnv(std::vector<std::string> env_vector);
		void	childProcess(char **args, int &pipe_in);
		void	parentProcess(int &pipe_out);
		void	addEnvVars(void);

	public:
		CGI();
		CGI(Http_req const &request);
		~CGI(){};
		CGI(CGI const &copy);

		void	executeCGI();
		
};