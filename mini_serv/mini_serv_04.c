#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

fd_set	active_fds, read_fds, write_fds;
int		max_fd;
int		ids[65536];
char	*msgs[65536];
int		count = 0;
char	buf_write[64];
char	buf_read[1001];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void fatal_error(void)
{
	write(2, "Fatal error\n", 12);
	exit(1);
}

void notify_other(int author, char *str)
{
	for (int i = 0; i <= max_fd; ++i)
	{
		if (FD_ISSET(i, &write_fds) && i != author)
			send(i, str, strlen(str), 0);
	}
}

void remove_client(int fd)
{
	sprintf(buf_write, "server: client %d just left\n", ids[fd]);
	notify_other(fd, buf_write);

	free(msgs[fd]);
	msgs[fd] = NULL;

	FD_CLR(fd, &active_fds);
	close(fd);
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		write(2, "caca\n", 5);
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

					if (client_fd > 0)
					{
						FD_SET(client_fd, &active_fds);
						if (client_fd > max_fd)
							max_fd = client_fd;
						ids[client_fd] = count++;
						msgs[client_fd] = NULL;
						sprintf(buf_write, "server: client %d just arrived\n", ids[client_fd]);
						notify_other(client_fd, buf_write);
					}
				}
				else
				{
					int	bytes = recv(i, buf_read, 1000, 0);
					
					if (bytes > 0)
					{
						buf_read[bytes] = '\0';
						char *new_msg = str_join(msgs[i], buf_read);
						if (new_msg == NULL)
							fatal_error();
						msgs[i] = new_msg;

						int result;
						char *msg;
						while ((result = extract_message(&msgs[i], &msg)) != 0)
						{
							if (result == -1)
								fatal_error();

							sprintf(buf_write, "client %d: ", ids[i]);
							notify_other(i, buf_write);
							notify_other(i, msg);
							free(msg);
						}
					}
					else
					{
						remove_client(i);
						break;
					}
				}
			}
		}
	}

	return (0);
}
