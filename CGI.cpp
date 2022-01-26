#include "CGI.hpp"

CGI::CGI()
{
	extern char **environ;

	_env_vec = envToVector(environ);
}

CGI::CGI(Http_req *request, const server_location *s, const server_info *info) :
	_serv_loc(s),
	_request(request),
	_path_cgi(_serv_loc->root),
	_name_cgi(_serv_loc->root + _request->file_uri),
	_info(info)
{
	extern char **environ;

	_env_vec = envToVector(environ);
	if (!(_args = (char **)malloc(sizeof(char *) * 3)))
		throw 500;
	std::string aux = this->_request->file_uri;
	aux = aux.substr(aux.find_last_of('.'), aux.npos);
	std::map<std::string, std::string>::const_iterator it = _info->cgi_paths->find(aux);
	if (it == _info->cgi_paths->end())
		throw 500;
	_args[0] = strdup(it->second.c_str());
	_args[1] = strdup(_name_cgi.c_str());
	_args[2] = NULL;
}

CGI::~CGI()
{
	free(_args[0]);
	free(_args[1]);
	free(_args);
}

CGI::CGI(CGI const &copy) :
	_serv_loc(copy._serv_loc),
	_request(copy._request),
	_path_cgi(copy._path_cgi),
	_name_cgi(copy._name_cgi),
	_response_cgi(copy._response_cgi),
	_CGI_fd(copy._CGI_fd),
	_env_vec(copy._env_vec),
	_info(copy._info)
{}

std::string	CGI::executeCGI()
{
	int		status, valwrite;
	int		pipes[2];
	pid_t	pid;
	
	if ((_CGI_fd = open("./cgi.temp", O_RDWR | O_CREAT | O_TRUNC | O_NOFOLLOW | O_NONBLOCK, 0666)) < 0)
		throw 500;
	valwrite = write(this->_CGI_fd, _request->body.c_str(), _request->body.length());
	close(_CGI_fd);
	if (pipe(pipes))
		throw 500;
	addEnvVars();
	if ((pid = fork()) < 0)
		throw 500;
	else if (pid == 0)
		childProcess(_args, pipes[SIDE_IN]);
	else
	{
		waitpid(pid, &status, 0);
		parentProcess(pipes[SIDE_OUT]);
	}
	unlink("./cgi.temp");
	close(pipes[SIDE_IN]);
	close(pipes[SIDE_OUT]);
	return (_response_cgi);
}

void CGI::childProcess(char **args, int &pipes_in)
{
	int	i = 0, ret;
	char **env = vectorToEnv(_env_vec);

	if (dup2(pipes_in, STDOUT) < 0)
	{
		std::cout << "Status: 500 Internal Server Error\r\n\r\n";
		exit(1);
	}
	if (_request->body.length() > 0)
	{
		_CGI_fd = open("./cgi.temp", O_RDONLY, 0);
		if (dup2(_CGI_fd, STDIN))
		{
			std::cout << "Status: 500 Internal Server Error\r\n\r\n";
			exit(1);
		}
	}
	else
		close(STDIN);
	if (_request->body.length() > 0)
		close(_CGI_fd);
	if (!fileExists(args[1]) || (ret = execve(args[0], args, env)) < 0)
		ret = 1;
	std::cout << "Status: 500 Internal Server Error\r\n\r\n";
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
		std::string temp(env[i]);
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
	_env_vec.push_back("SERVER_NAME="  + _info->names[0]);
	_env_vec.push_back("SERVER_PORT=" + _info->port);
	_env_vec.push_back("SERVER_SOFTWARE=webserv/1.0");
	_env_vec.push_back("SERVER_PROTOCOL=HTTP/1.1");
	_env_vec.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env_vec.push_back("REDIRECT_STATUS=200");
	_env_vec.push_back("REQUEST_URI=" + _request->uri);
	_env_vec.push_back("REQUEST_METHOD=" + _request->method);
	_env_vec.push_back("AUTH_TYPE=NULL");
	_env_vec.push_back("REMOTE_USER=NULL");
	if (_request->head.count("content-type"))
		_env_vec.push_back("CONTENT_TYPE=" + _request->head["Content-Type"]);
	if (_request->head.count("Content-Length"))
		_env_vec.push_back("CONTENT_LENGTH=" + _request->head["Content-Length"]);
	else
		_env_vec.push_back("CONTENT_LENGTH=0");
	_env_vec.push_back("QUERY_STRING=" + _request->query_string);
	_env_vec.push_back("PATH_INFO=" + _request->uri);
	_env_vec.push_back("PATH_TRANSLATED=" + _path_cgi);
	_env_vec.push_back("SCRIPT_NAME=" + _name_cgi);
	_env_vec.push_back("SCRIPT_FILENAME=" + _name_cgi);
	_env_vec.push_back("REMOTE_ADDR=127.0.0.1");
	_env_vec.push_back("REMOTE_IDENT=local");
}
