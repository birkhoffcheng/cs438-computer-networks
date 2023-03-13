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
#include <errno.h>
#include <stdbool.h>
#include "packet.h"

struct sockaddr_in si_other;
int s, slen;

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

	unsigned char *buf = malloc(bytesToTransfer);
	unsigned char *packet;
	size_t bytes_to_send;
	ssize_t bytes_written, bytes_read;
	uint32_t seq = 1, ack = 1, last_ack = 1, window = MAX_PAYLOAD_SIZE;
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if (!buf)
		diep("malloc");
	fread(buf, 1, bytesToTransfer, fp);
	fclose(fp);

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

	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
		diep("setsockopt");

	if (connect(s, (struct sockaddr *) &si_other, sizeof(si_other)) < 0)
		diep("connect");

	/* Send data and receive acknowledgements on s*/
	bool dup_ack;
	while ((last_ack - 1) < bytesToTransfer) {
		while (seq < ack + window) {
			bytes_to_send = min(MAX_PAYLOAD_SIZE, bytesToTransfer - (seq - 1));
			if (bytes_to_send == 0 ) {
				break;
			}
			printf("sending packet seq %u ack %u window %u\n", seq, ack, window);
			packet = make_packet(seq, buf + (seq - 1), bytes_to_send);
			bytes_written = write(s, packet, bytes_to_send + HEADER_LENGTH);
			free(packet);
			if (bytes_written < 0)
				diep("write");
			seq += bytes_to_send;
		}
		dup_ack = false;
		while (last_ack < seq && (bytes_read = recvfrom(s, &ack, 4, 0 ,NULL, NULL)) > 0) {
			if (bytes_read < 4)
				break;
			ack = ntohl(ack);
			printf("received ack %u\n", ack);
			if (ack > last_ack) {
				last_ack = ack;
			}
			else {
				seq = last_ack;
				window = max(window / 2, MAX_PAYLOAD_SIZE);
				dup_ack = true;
				break;
			}
		}
		if (!dup_ack) {
			// increase sending window
			if (window < (MAX_PAYLOAD_SIZE * SSTHRESHOLD))
				window *= 2;
			else
				window += MAX_PAYLOAD_SIZE;
		}
		if (bytes_read == -1 && errno == EAGAIN) {
			window = MAX_PAYLOAD_SIZE;
			seq = last_ack;
		}
	}

	buf[0] = 0;
	write(s, buf, 1);
	free(buf);
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
