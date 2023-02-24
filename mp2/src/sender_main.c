#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include "packet.h"

struct sockaddr_in si_other;
int s, slen;
struct packet_node *head, *tail;

void diep(char *s) {
	perror(s);
	exit(1);
}

unsigned char *make_packet(uint32_t seq, unsigned char *data, uint32_t len) {
	unsigned char *buf = malloc(len + HEADER_LENGTH);
	seq = htonl(seq);
	memcpy(buf, &seq, sizeof(seq));
	memcpy(buf + sizeof(seq), data, len);
	return buf;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
	//Open the file
	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("Could not open file to send.");
		exit(1);
	}

	/* Determine how many bytes to transfer */

	slen = sizeof (si_other);

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_other, 0, sizeof (si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(hostUDPport);
	if (inet_aton(hostname, &si_other.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	if (bind(s, (struct sockaddr *) &si_other, sizeof(si_other)) < 0)
		diep("bind");

	/* Send data and receive acknowledgements on s*/

	printf("Closing the socket\n");
	close(s);
	return;

}

/*
 *
 */
int main(int argc, char** argv) {

	unsigned short int udpPort;
	unsigned long long int numBytes;

	if (argc != 5) {
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int) atoi(argv[2]);
	numBytes = atoll(argv[4]);



	reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


	return (EXIT_SUCCESS);
}
