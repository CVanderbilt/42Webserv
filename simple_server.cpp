#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <cstdlib> // For exit() and EXIT_FAILURE
#include <iostream> // For cout
#include <unistd.h> // For read
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>

#define PORT 9999

int		main()
{
	int					server_fd;
	int					new_fd;
	int 				numbytes;
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
//	std::cout << read_buffer << std::endl;
    
    if ((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
	if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
	while(1)
    {
        std::cout << "\n+++++++ Waiting for new connection ++++++++\n\n";
        if ((new_fd = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }
        if (send(new_fd , read_buffer, read_file, 0) == -1)
		{
            perror("In send");
            exit(EXIT_FAILURE);
        }
        char recv_buffer[4096];
        numbytes = recv(new_fd , recv_buffer, sizeof(recv_buffer), 0);
        if (numbytes == 0)
        	std::cout << "server: hung up\n";
        else if (numbytes < 0)
		{
            perror("In recv");
            exit(EXIT_FAILURE);
        }
		std::cout << recv_buffer << std::endl;
        
        std::cout << "------------------ message sent-------------------\n";
        close(new_fd);
    }
    return 0;
}