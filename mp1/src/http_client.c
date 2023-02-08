/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// char *strncpy( char *dest, const char *src, size_t n )
void parse_input(char * input, char * ip, char * path, char * port) {
	char * s;
	s = input;
	while (*s != '/') { // this is to skip the http:// before useful inputs
		s = s + 1;
	}
	s = s + 2; // now s is at start of ip
	char * p;
	p = s;
	int port_found = 0;
	char * port_start;
	char * path_start;
	while (*p != 0) {
		if (*p == ':') { // if there is a port, before : is ip address 
			strncpy(ip, s, p - s); // record ip address in to buffer
			port_found = 1;
			port_start = p;
		}
		if (*p == '/') {
			if (port_found == 1) { // if there is a port we scan over, we are at the / immediately after port, thus, 
				strncpy(port, port_start, p - port_start);
				path_start = p;
				break;
			} 
			else 
			{ // if there is no port, then first / means the end of ip address
				strncpy(ip, s, p - s);
				path_start = p;
				break;
			}
		}
		p++;
	}
	while (*p != 0) {
		p++;
	}
	if (port_found == 0) {
		*port = 80;
	}
	strncpy(path, path_start, p - path_start);
}


int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	char path[2048];
	char ip[2048];
	char port[8];
	memset(path, 0, sizeof(path));
	memset(ip, 0, sizeof(ip));
	memset(port, 0, sizeof(port));
	parse_input(argv[1], ip, path, port);
	//printf("\n %s, %s, %s", path, ip, port);
	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}