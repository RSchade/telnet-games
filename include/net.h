#ifndef __NET_H
#define __NET_H

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
 #include <poll.h>
#include "main.h"


void socket_poll();
int socket_init();
void net_cleanup();
ssize_t write_str(int socket, char *buf);
ssize_t read_str(int socket, char **buf);

#endif