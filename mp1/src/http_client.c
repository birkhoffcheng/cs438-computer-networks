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
#define GET_HEADER "GET %s HTTP/1.1\r\n\r\n"
// GET /test.txt HTTP/1.1
// User-Agent: Wget/1.12 (linux-gnu)
// Host: localhost:3490
// Connection: Keep-Alive
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
		if (*p == ':') { // if there is a port, before : is ip address 
			strncpy(ip, s, p - s); // record ip address in to buffer
			port_found = 1;
			port_start = p + 1;
		}
		p += 1;
	}
	while (*p != 0) {
		p += 1;
	}
	strncpy(path, path_start, p - path_start);
}


int main(int argc, char *argv[])
{
	int sockfd;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];


	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	char path[512];
	char ip[512];
	char port[8];
	memset(path, 0, sizeof(path));
	memset(ip, 0, sizeof(ip));
	memset(port, 0, sizeof(port));
	parse_input(argv[1], ip, path, port);
	if (*port == 0) {
		strcpy(port, "80");
	}
	printf("\n %s, %s, %s", path, ip ,port);
	char request[2048];
	memset(request, 0, sizeof(request));
	sprintf(request, GET_HEADER, path);
	int length = strlen(request);
	printf("\n%s", request);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* ________________________________________________________________*/
	// connect to the server
	if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
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
	/*_________________________________________________________________________*/
	// end of connecting
	// start sending the header to the server
	int total_sent = 0;

	// send the header to the server
	while(total_sent < length) {
		int sent = send(sockfd, request, length, 0);
		if (sent == -1) {
			perror("sending:failure");
		}
		total_sent = total_sent + sent;
	}

	// now all message sent to server
	// hanging for server to send its header and the requested file
	// write the output file to buf
	memset(buf, 0, sizeof(buf));


	FILE * output = fopen("output", "w");
	int pass_header = 0;
	int rec = 1;
	while ((rec = recv(sockfd, buf, MAXDATASIZE - 1, 0)) != 0) {
		// rec has the number of bytes received
		if (rec == -1) {
			return 1;
		}
		if (pass_header == 0) {
			pass_header = 1;
			char * header_end = strstr(buf, "\r\n\r\n");
			header_end += 4;
			fwrite(header_end, 1, buf - header_end + rec, output);
		}
		if (pass_header == 1) {
			fwrite(buf, 1, rec, output);
		}

	}

	// NOTE: this way causes stack overflow because all of the file in the stack
	// char rec = 0;
	// char * buf_start = buf;
	// char * buf_traverse = buf;
	// while (1) {
	// 	rec = recv(sockfd, buf_traverse, MAXDATASIZE-1, 0);
	// 	if (rec == 0) {
	// 		break;
	// 	}
	// 	char * new = buf_traverse + rec;
	// 	buf_traverse = new;

	// }
	// char * skip_header = strstr(buf_start, "\r\n\r\n");
	// char * content_length = strstr(buf_start, "Content-Length: ");
	// content_length += 16;
	// char file_len[8];
	// memset(file_len, 0, sizeof(file_len));
	// strncpy(file_len, content_length, skip_header - content_length);
	// int len = atoi(file_len);
	// printf("\n file_len is %d", len);
	// skip_header += 4;
	// // now all I need to do is to first fetch the header field
	// // int file_len = strlen(skip_header);
	// fwrite(skip_header, 1, len , output);



	// if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	//     perror("recv");
	//     exit(1);
	// }

	// buf[numbytes] = '\0';

	// printf("client: received '%s'\n",buf);
	fclose(output);
	close(sockfd);

	return 0;
}