#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <fstream> // For files
#include <iostream> 
#include <unistd.h> // For read
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include "Http_req.hpp"

#define PORT 9999
#define BUFFER_SIZE 4096

int		main()
{
	const std::string response = "HTTP/1.1 200 OK\r\n"
							"Date: Sun, 18 Oct 2009 10:47:06 GMT\r\n"
							"Server: Apache/2.2.14 (Win32)\r\n"
							"Last-Modified: Sat, 20 Nov 2004 07:16:26 GMT\r\n"
							"ETag: \"10000000565a5-2c-3e94b66c2e680\"\r\n"
							"Accept-Ranges: bytes\r\n"
							"Content-Length: 44\r\n"
							"Keep-Alive: timeout=5, max=100\r\n"
							"Connection: Keep-Alive\r\n"
							"Content-Type: text/html\r\n"
							"\r\n" 
							"<html><body><h1>It works!</h1></body></html>";

	int					server_fd;
	int					new_fd;
	int 				numbytes;
	struct	sockaddr_in	addr;
	socklen_t			addrlen = sizeof(addr);

	char 				*buffer = new char [BUFFER_SIZE + 1];

	if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("In socket");
		exit(EXIT_FAILURE);
	}
	if ((fcntl(server_fd, F_SETFL, O_NONBLOCK)) == -1)
	{
		perror("In fcntl");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr))<0)
	{
		perror("In bind");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 10) < 0)
	{
		perror("In listen");
		close(server_fd);
		exit(EXIT_FAILURE);
	}
	pollfd *pfds = new pollfd[10];
	Http_req *reqs = new Http_req[10];

	pfds[0].fd = server_fd;
	pfds[0].events = POLLIN | POLLOUT;
	int fd_count = 1;

	while(1)
	{
		int poll_count = poll(pfds, fd_count, -1);
		if (poll_count == -1)
		{
			perror("In poll");
			close(server_fd);
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < fd_count; i++)
		{
			if (pfds[i].revents & POLLIN)
			{
                if (pfds[i].fd == server_fd)
				{
					new_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
					if (new_fd == -1)
						perror("In accept");
					else
					{
						pfds[fd_count].fd = new_fd;
    					pfds[fd_count].events = POLLIN;
    					fd_count++;
						std::cout << "pollserver: new connection on socket " << new_fd << std::endl;
					}
				}
				else
				{
					if ((numbytes = recv(pfds[i].fd, buffer, BUFFER_SIZE, 0)) < 0)
						perror("In recv");
					int status = reqs[i].parse_chunk(buffer);
					std::cout << "status: " << Http_req::status_to_str((Http_req::e_parsing_status)status) << std::endl;
					switch ((Http_req::e_parsing_status)status)
					{
						case Http_req::PARSE_END:
							send(pfds[i].fd, response.c_str(), response.length(), 0);
							std::cout << reqs[i] << std::endl;
							return (0); //for testing purposes will return instead of reset fd
						case Http_req::PARSE_ERROR:
							return (1); //for testing purposes will return instead of reset fd and send error
						default:
							continue ;
					}
				}
			}
		}
	}
	return 0;
}