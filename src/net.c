#include "../include/net.h"

int socket_desc;
struct sockaddr_in server, client;

int socket_init() {
	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}

    // Reuse the address
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc, 3);

    fcntl(socket_desc, F_SETFL, O_NONBLOCK);

    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    return 0;
}

struct pollfd *open_socks = NULL;
nfds_t nfds = 0;
nfds_t nfds_size = 1;

void socket_poll() {
    int c, new_socket;
    c = sizeof(struct sockaddr_in);
    short sock_events = POLLIN | POLLHUP;

    open_socks = calloc(sizeof(struct pollfd), nfds_size);
    open_socks[0].fd = socket_desc;
    open_socks[0].events = sock_events;
    nfds++;

    //unsigned long n = 0;

    while (1) {
        // wait until something happens
        poll(open_socks, nfds, 10000);
        //printf("REFRESH %ld %ld\r\n", n, nfds);
        // check for new connection
        new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        uint8_t e_again = new_socket < 0 && errno == (EAGAIN | EWOULDBLOCK);
        if (!e_again) {
            fcntl(new_socket, F_SETFL, O_NONBLOCK);
            if (nfds_size == nfds) {
                printf("INCREASE POLLFD SOCK SIZE FROM %ld TO %ld\r\n", nfds_size, nfds_size + 5);
                nfds_size += 5;
                open_socks = realloc(open_socks, sizeof(struct pollfd) * nfds_size);
            }
            open_socks[nfds].fd = new_socket;
            open_socks[nfds].events = sock_events;
            nfds++;
            char *ip = inet_ntoa(client.sin_addr);
            printf("Connection accepted from %s on port %d\r\n", ip, client.sin_port);
            char *id = malloc(sizeof(char) * (strlen(ip) + 7));
            sprintf(id, "%s:%d", ip, client.sin_port);
            new_user(new_socket, id, client);
        }
        // run the event loop
        event_loop();
        //n++;
    }
}

void remove_socket(int socket) {
    // handle nfds deductions
    printf("SOCKET %d DISCONNECTED... REMOVING\r\n", socket);
    for (nfds_t i = 0; i < nfds; i++) {
        if (open_socks[i].fd == socket) {
            // take out of the array,
            // move everything back over this to 'cover up'
            // the invalid record
            for (nfds_t j = i; j < nfds - 1; j++) {
                open_socks[j] = open_socks[j + 1];
            }
            nfds--;
            break;
        }
    }
}

ssize_t write_str(int socket, char *buf) {
    ssize_t written = write(socket, buf, strlen(buf));
    if (written == -1) {
        printf("Write failed with error:\r\n%s\r\n", strerror(errno));
        if (errno == EPIPE) {
            remove_socket(socket);
        }
    }
    return written;
}

ssize_t read_str(int socket, char **buf) {
    // recieve bytes from the client
    ssize_t buf_size = 10;
    ssize_t read_len = 0;
    *buf = malloc(sizeof(char) * buf_size);
    uint8_t e_again = 0;
    while (!e_again) {
        ssize_t size_read = read(socket, *buf + read_len, buf_size - read_len);
        if (size_read > 0) {
            read_len += size_read;
            buf_size *= 2;
            *buf = realloc(*buf, buf_size);
            for (ssize_t i = read_len; i < buf_size; i++) {
                (*buf)[i] = '\x00';
            }
        } else {
            e_again = (errno & (EAGAIN | EWOULDBLOCK)) > 0;
            if (size_read == 0) {
                remove_socket(socket);
                free(*buf);
                return -1;
            }
        }
    }
    return read_len;
}

void disc_socket(int socket) {
    remove_socket(socket);
    close(socket);
}

void close_sock() {
    close(socket_desc);
}