#include "../include/net.h"

int socket_desc;
struct sockaddr_in server, client;

int socket_init() {
    int new_socket;
	
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

void socket_poll() {
    int c, new_socket;
    c = sizeof(struct sockaddr_in);

    while (1) {
        // check for new connection
        new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        uint8_t e_again = new_socket < 0 && errno == (EAGAIN | EWOULDBLOCK);
        if (!e_again) {
            fcntl(new_socket, F_SETFL, O_NONBLOCK);
            char *ip = inet_ntoa(client.sin_addr);
            printf("Connection accepted from %s on port %d\n", ip, client.sin_port);
            char *id = malloc(sizeof(char) * (strlen(ip) + 7));
            sprintf(id, "%s:%d", ip, client.sin_port);
            new_user(new_socket, id, client);
        }
        // poll (maybe do select?)
        event_loop();
    }
}

ssize_t write_str(int socket, char *buf) {
    ssize_t written = write(socket, buf, strlen(buf));
    if (written == -1) {
        printf("Write failed with error:\n%s\n", strerror(errno));
    }
    return written;
}

void close_sock() {
    close(socket_desc);
}