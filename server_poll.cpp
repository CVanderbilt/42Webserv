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
					int status = reqs->parse_chunk(buffer);
					switch (status)
					{
					case Http_req::PARSE_ERROR:
						break;
					case Http_req::PARSE_ONGOING:
						break;
					case Http_req::PARSE_HEAD:
						break;
					case Http_req::PARSE_END:
						send(pfds[i].fd, "OK", sizeof("OK"), 0);
						break;
					default:
						break;
					}
				}
			}
		}
	}
	return 0;
}