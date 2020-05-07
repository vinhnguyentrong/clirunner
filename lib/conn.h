#ifndef __CONN_H__
#define __CONN_H__

#include <stdint.h>
struct conn {
	char *msg_r;
	char *msg_w;
	size_t msg_w_len_max;
	size_t msg_r_len_max;
	int fd_client;
	int fd_server_group;
};


struct conn *conn_init(const char *SERVER_ADDR, int PORT);

void conn_free(struct conn *conn);

int readall(int fd, char *buffer, int size);

void *check_conn();

extern struct conn *conn;

#endif
