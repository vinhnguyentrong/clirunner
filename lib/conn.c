#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <sys/socket.h>

#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "conn.h"

#define BUF_SIZE (1024 * 1024)
#define MSG_W_MAX (1024)
#define MSG_R_MAX (1024 * 1024)


int readall(int fd, char *buffer, int size) 
{
	int num_read, total_read;
	char *buf;
	buf = buffer;
	for (total_read = 0;; ) {

		num_read = read(fd, buf, size - total_read);
		if (num_read == 0)
			return total_read;
		if (num_read == -1) {
			if (errno == EINTR)
				continue; 		// interrupted -> restart read()
			else
				return -1;
		}
		total_read += num_read;
		if (buf[num_read - 2] == '>') {
			return total_read;
		}
		buf += num_read;
	}
	return total_read;
}

struct conn *conn_init(const char *SERVER_ADDR, int PORT) {

	struct sockaddr_in server_addr;
	struct conn *conn;
	int fd_client, status, ret, fd_server_group;

	conn = calloc(1, sizeof(struct conn));
	if (conn == NULL)
		return NULL;

	conn->msg_r = calloc(1, MSG_R_MAX);
	conn->msg_w = calloc(1, MSG_W_MAX);

	if ((conn->msg_r == NULL) ||
		(conn->msg_w == NULL)) {
		conn_free(conn);
		return NULL;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	server_addr.sin_port = htons(PORT);

	//create socket
	fd_client = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_client == -1) {
		conn_free(conn);
		return NULL;
	}

	// Establish connection
	status = connect(fd_client, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (status < 0) {
		perror("Cannot connect to the server");
		conn_free(conn);
		return NULL;
	}

	fd_server_group = epoll_create(1);
	if (fd_server_group == -1) {
		conn_free(conn);
		close(fd_client);
		return NULL;
	}

	conn->msg_w_len_max = MSG_W_MAX;
	conn->msg_r_len_max = MSG_R_MAX;
	conn->fd_server_group = fd_server_group;
	conn->fd_client = fd_client;

	ret = readall(fd_client, conn->msg_r, conn->msg_r_len_max);
	if (ret > 0) {
		conn->msg_r[ret] = '\0';
		printf("%s\n", conn->msg_r);
		fflush(stdout);
	}

	return conn;
}

void conn_free(struct conn *conn)
{
	if (conn == NULL)
		return;
	if (conn->fd_server_group)
		close(conn->fd_server_group);
	if (conn->fd_client)
		close(conn->fd_client);

	free(conn->msg_r);
	free(conn->msg_w);
	free(conn);
}

