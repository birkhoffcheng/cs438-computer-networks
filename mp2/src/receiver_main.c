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
#include "packet.h"


struct sockaddr_in si_me, si_other;
int s;
socklen_t slen;

void diep(char *s) {
	perror(s);
	exit(1);
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

	slen = sizeof (si_other);
	FILE *fp = fopen(destinationFile, "wb");
	if (!fp)
		diep("fopen");

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		diep("socket");

	memset((char *) &si_me, 0, sizeof (si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(myUDPport);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	printf("Now binding\n");
	if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
		diep("bind");

	unsigned char buf[MTU];
	ssize_t bytes_read;
	uint32_t seq, ack = 1, ack_n = htonl(1);

	/* Now receive data and send acknowledgements */
	while ((bytes_read = recvfrom(s, buf, MTU, 0, (struct sockaddr *) &si_other, &slen)) > 4) {
		memcpy(&seq, buf, 4);
		seq = ntohl(seq);
		bytes_read -= 4;
		if (seq != ack) {
			sendto(s, &ack_n, 4, 0, (struct sockaddr *) &si_other, slen);
			continue;
		}
		ack = seq + bytes_read;
		ack_n = htonl(ack);
		sendto(s, &ack_n, 4, 0, (struct sockaddr *) &si_other, slen);
		fwrite(buf + 4, 1, bytes_read, fp);
	}

	close(s);
	printf("%s received %u bytes\n", destinationFile, ack - 1);
	fclose(fp);
	return;
}

/*
 *
 */
int main(int argc, char** argv) {

	unsigned short int udpPort;

	if (argc != 3) {
		fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
		exit(1);
	}

	udpPort = (unsigned short int) atoi(argv[1]);

	reliablyReceive(udpPort, argv[2]);
}
