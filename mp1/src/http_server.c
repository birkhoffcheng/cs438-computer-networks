#include <arpa/inet.h>
#include <ctype.h>
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
	fprintf(stderr, "Caught signal %d: %s\n", signum, strsignal(signum));
	pid_t pid = wait(&ws);
	fprintf(stderr, "Child %d exited with status %d\n", pid, WEXITSTATUS(ws));
}

char **split(char *line, char *delim, size_t *size) {
	size_t n = 0, sz = 4;
	char *save = line, *token, **tokens = malloc(sizeof(char *) * sz);
	while ((token = strtok_r(NULL, delim, &save))) {
		if (n >= sz) {
			sz *= 2;
			tokens = realloc(tokens, sizeof(char *) * sz);
		}
		for (; isblank(*token); token++);
		tokens[n++] = token;
	}
	if (size != NULL)
		*size = n;
	return tokens;
}

void serve_file(int fd, char *path) {
	int file;
	ssize_t bytes_sent = 0, bytes;
	struct stat file_stat;
	char file_size_buffer[32];

	write(fd, "HTTP/1.0 200 OK\r\nContent-Length: ", 33);
	stat(path, &file_stat);
	file = snprintf(file_size_buffer, 32, "%ld", file_stat.st_size);
	write(fd, file_size_buffer, file);
	write(fd, "\r\n\r\n", 4);

	file = open(path, O_RDONLY);
	if (file < 0) {
		perror("open");
		exit(errno);
	}

	while (bytes_sent < file_stat.st_size) {
		bytes = sendfile(fd, file, NULL, file_stat.st_size - bytes_sent);
		if (bytes < 0) {
			perror("sendfile");
			exit(errno);
		}
		bytes_sent += bytes;
	}

	close(file);
}

void handle_request(int fd) {
	char read_buffer[BUFSIZ];

	read(fd, read_buffer, BUFSIZ);
	size_t size;
	char **lines = split(read_buffer, "\n", &size);
	if (size == 0) {
		write(fd, "HTTP/1.0 400 Bad Request\r\n\r\n", 28);
		goto free_lines;
	}

	char **request = split(lines[0], " ", &size);
	if (size < 2 || request[1][0] != '/') {
		write(fd, "HTTP/1.0 400 Bad Request\r\n\r\n", 28);
		goto free_request;
	}

	if (strstr(request[1], "..")) {
		write(fd, "HTTP/1.0 403 Forbidden\r\n\r\n", 26);
		goto free_request;
	}

	if (strcmp(request[0], "GET")) {
		write(fd, "HTTP/1.0 405 Method Not Allowed\r\n\r\n", 35);
		goto free_request;
	}

	char *path = request[1] + 1;

	if (path[0] == '\0') {
		path[0] = '.';
		path[1] = '\0';
	}

	struct stat file_stat;
	if (stat(path, &file_stat) == -1) {
		write(fd, "HTTP/1.0 404 Not Found\r\n\r\n", 26);
		goto free_request;
	}

	serve_file(fd, path);

free_request:
	free(request);
free_lines:
	free(lines);
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
