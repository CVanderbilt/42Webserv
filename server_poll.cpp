#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <iostream> // For cout
#include <unistd.h> // For read
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>
#include <vector>

#define PORT 9999

int		main()
{
	int					server_fd;
	int					new_fd;
	int 				numbytes;
	char				buffer[256];
	struct	sockaddr_in	addr;
	socklen_t			addrlen = sizeof(addr);
	std::string			message = "Here the server\n";

	int 				read_fd;
	int 				read_file;
    char                read_buffer[4096];
    if ((read_fd = open("http", O_RDONLY)) < 0)
	{
        perror("In open");
        exit(EXIT_FAILURE);
    }
    if ((read_file = read(read_fd, read_buffer, sizeof(read_buffer))) < 0)
    {
        perror("In read file");
        exit(EXIT_FAILURE);
    }
	std::cout << read_buffer << std::endl;
	close(read_fd);

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

	pfds[0].fd = server_fd;
	pfds[0].events = POLLIN;
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
					{
						perror("In accept");
						close(server_fd);
						close(new_fd);
						exit(EXIT_FAILURE);
					}
					else
					{
						pfds[fd_count].fd = new_fd;
    					pfds[fd_count].events = POLLIN;
    					fd_count++;
						std::cout << "pollserver: new connection on socket " << new_fd << std::endl;
						if (send(new_fd , read_buffer, read_file, 0) == -1)
						{
							perror("In send");
							close(server_fd);
							close(new_fd);
							exit(EXIT_FAILURE);
						}
					}
					close(new_fd);
				}
			}
		}
	}
	return 0;
}