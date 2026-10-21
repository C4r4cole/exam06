#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

void fatal_error(void)
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	int sockfd;
	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		fatal_error();

	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();
	if (listen(sockfd, 10) != 0)
		fatal_error();

	fd_set	active_fds;
	fd_set	read_fds;
	fd_set	write_fds;
	int		max_fd;

	FD_ZERO(&active_fds);

	FD_SET(sockfd, &active_fds);
	max_fd = sockfd;

	while (1)
	{
		write_fds = read_fds = active_fds;

		if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
			fatal_error();

		for (int i = 0; i <= max_fd; ++i)
		{
			if (FD_ISSET(i, &read_fds))
			{
				if (i == sockfd)
				{
					int client_fd = accept(i, NULL, NULL);
					
					if (client_fd >= 0)
					{
						FD_SET(client_fd, &active_fds);
						if (client_fd > max_fd)
							max_fd = client_fd;
					}
				}
			}
		}
	}

	return (0);
}