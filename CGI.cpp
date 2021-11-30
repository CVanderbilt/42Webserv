#include "CGI.hpp"

CGI::CGI()
{
	extern char **environ;

	_env_vec = envToVector(environ);
}

CGI::CGI(Http_req const &request) : _request(request)
{
	extern char **environ;

	_env_vec = envToVector(environ);
}

CGI::CGI(CGI const &copy) :
	_path_cgi(copy._path_cgi),
	_response_cgi(copy._response_cgi),
	_request(copy._request)
{}

void	CGI::executeCGI()
{
	int		status, valwrite;
	int		pipes[2];
	pid_t	pid;
	char	**args;

	if (!(args = (char **)malloc(sizeof(char *) * 3)))
		throw 500;
	if ((_CGI_fd = open("./cgi.temp", O_RDWR | O_CREAT | O_TRUNC | O_NOFOLLOW | O_NONBLOCK, 0666)) < 0)
		throw 500;
	valwrite = write(this->_CGI_fd, _request.body.c_str(), _request.body.length());
	close(this->_CGI_fd);
	if (pipe(pipes))
		throw 500;
	args[0] = strdup(_path_cgi.c_str());
	args[1] = strdup(_name_cgi.c_str());
	args[2] = NULL;
	if ((pid = fork()) < 0)
		throw 500;
	else if (pid == 0)
		childProcess(args, pipes[SIDE_IN]);
	else
	{
		waitpid(pid, &status, 0);
		parentProcess(pipes[SIDE_OUT]);
	}
	free(args[0]);
	free(args[1]);
	free(args);
	unlink("./cgi.temp");
	close(pipes[SIDE_IN]);
	close(pipes[SIDE_OUT]);
}

void CGI::childProcess(char **args, int &pipes_in)
{
	int	i = 0, cgi_fd, ret;
	char **env = vectorToEnv(_env_vec);

	if (dup2(pipes_in, STDOUT) < 0)
		throw 500;
	if (_request.body.length() > 0)
	{
		this->_CGI_fd = open("./cgi.temp", O_RDONLY, 0);
		if (dup2(this->_CGI_fd, STDIN))
			throw 500;
	}
	else
		close(STDIN);
	if (this->_request.body.length() > 0)
		close(cgi_fd);
	if ((ret = execve(args[0], args, env)) < 0)
		throw 500;
	while (env[i])
		free(env[i++]);
	free(env);
	exit(ret);
}

void	CGI::parentProcess(int &pipe_out)
{
	int		valread, poll_count;
	char	buffer[BUFFER_SIZE + 1] = {0};
	pollfd	pfds[1];

	_response_cgi = "";
	pfds[0].fd = pipe_out;
    pfds[0].events = POLLIN;
	while ((poll_count = poll(pfds, 1, 0)))
	{
		if ((valread = read(pipe_out, buffer, BUFFER_SIZE)) <= 0)
			continue;
		_response_cgi += std::string(buffer, valread);
	}
}

std::vector<std::string> CGI::envToVector(char **env)
{
	std::vector<std::string> vec;

	for (int i = 0; env[i]; i++)
	{
		std::string temp = "";
		for (int j = 0; env[i][j]; j++)
			temp += env[i][j];
		vec.push_back(temp);
	}
	return (vec);
}

char	**CGI::vectorToEnv(std::vector<std::string> env_vector)
{
	char **ret;
	if (!(ret = (char **)malloc(sizeof(char *) * (env_vector.size() + 1))))
		perror("malloc");
	ret[env_vector.size()] = NULL;
	for (size_t i = 0; i < env_vector.size(); i++)
		ret[i] = strdup(env_vector[i].c_str());
	return (ret);
}

void	CGI::addEnvVars(void)
{
	_env_vec.push_back("SERVER_NAME=" /*TODO + getServername*/);
	_env_vec.push_back("SERVER_PORT=" /*TODO + getPort*/);
	_env_vec.push_back("SERVER_SOFTWARE=webserv/1.0");
	_env_vec.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env_vec.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env_vec.push_back("REDIRECT_STATUS=200");
	_env_vec.push_back("REQUEST_URI=" + _request.uri);
	_env_vec.push_back("REQUEST_METHOD=" + _request.method);
	_env_vec.push_back("AUTH_TYPE=NULL");
	_env_vec.push_back("REMOTE_USER=NULL");
	if (_request.head.count("content-type"))
		_env_vec.push_back("CONTENT_TYPE=" + _request.head["content-type"]);
	if (_request.head.count("content-length"))
		_env_vec.push_back("CONTENT_LENGTH=" + _request.head["content-length"]);
	else
		_env_vec.push_back("CONTENT_LENGTH=0");
	if (this->_request.uri.find('?') != std::string::npos)
		_env_vec.push_back("QUERY_STRING="/*TODO: + _query_string*/);
	else
		_env_vec.push_back("QUERY_STRING=");
	_env_vec.push_back("PATH_INFO=" + _request.uri);
	_env_vec.push_back("PATH_TRANSLATED=" + _path_cgi);
	_env_vec.push_back("SCRIPT_NAME=" + _name_cgi);
	_env_vec.push_back("SCRIPT_FILENAME=" + _name_cgi);
	_env_vec.push_back("REMOTE_ADDR=127.0.0.1");
	_env_vec.push_back("REMOTE_IDENT=local");

}