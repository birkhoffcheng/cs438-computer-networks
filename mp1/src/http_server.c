#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libgen.h>
#include <stdbool.h>
#include <sys/sendfile.h>

#define BACKLOG 1024
#define DEFAULT_PORT 3490

int server_fd;
void interrupt(int signum) {
	fprintf(stderr, "Caught signal %d: %s\n", signum, strsignal(signum));
	fprintf(stderr, "Closing socket %d\n", server_fd);
	close(server_fd);
	exit(EXIT_SUCCESS);
}

void collect_zombies(int signum) {
	int ws;
	pid_t pid = wait(&ws);
	fprintf(stderr, "Child %d exited with status %d\n", pid, WEXITSTATUS(ws));
}

void handle_request(int fd) {
	close(fd);
}

int main(int argc, char **argv) {
	struct sockaddr_in server_address, client_address;
	size_t client_address_length = sizeof(client_address);
	int client_socket_number;
	int server_port = DEFAULT_PORT;

	signal(SIGINT, interrupt);
	signal(SIGTERM, interrupt);
	signal(SIGCHLD, collect_zombies);

	if (argc > 1) {
		server_port = atoi(argv[1]);
		if (server_port <= 0 || server_port > 65535) {
			fprintf(stderr, "port number should be between 1 and 65535\n");
			exit(EXIT_FAILURE);
		}
	}

	server_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		exit(errno);
	}

	int socket_option = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) == -1) {
		perror("setsockopt");
		exit(errno);
	}

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(server_port);

	if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
		perror("bind");
		exit(errno);
	}

	if (listen(server_fd, BACKLOG) == -1) {
		perror("listen");
		exit(errno);
	}

	fprintf(stderr, "Listening on port %d\n", server_port);

	while (1) {
		client_socket_number = accept(server_fd, (struct sockaddr *) &client_address, (socklen_t *) &client_address_length);
		if (client_socket_number < 0) {
			perror("accept");
			continue;
		}

		fprintf(stderr, "Accepted connection from %s on port %d\n", inet_ntoa(client_address.sin_addr), client_address.sin_port);
		if (fork()) {
			close(client_socket_number);
		}
		else {
			close(server_fd);
			handle_request(client_socket_number);
			exit(EXIT_SUCCESS);
		}
	}
}
