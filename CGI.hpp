#pragma once

#include "Http_req.hpp"
#include "utils.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define SIDE_OUT 0
#define SIDE_IN 1
#define BUFFER_SIZE 4096

class CGI
{
	private:
		const server_location *_serv_loc;
		Http_req			_request;
		std::string			_path_cgi;
		std::string			_name_cgi;
		std::string			_response_cgi;
		int					_CGI_fd;
		std::vector<std::string> _env_vec;

		std::vector<std::string> envToVector(char **env);
		const server_info	*_info;
		char	**vectorToEnv(std::vector<std::string> env_vector);
		void	childProcess(char **args, int &pipe_in);
		void	parentProcess(int &pipe_out);
		void	addEnvVars(void);

	public:
		CGI();
		CGI(Http_req request, const server_location *s, const server_info *info);
		~CGI(){};
		CGI(CGI const &copy);

		std::string	executeCGI();
		
};