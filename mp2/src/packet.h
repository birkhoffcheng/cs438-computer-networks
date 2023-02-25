#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#define HEADER_LENGTH 4
#define MTU 1500
#define MAX_PAYLOAD_SIZE (MTU - 20 - 8 - HEADER_LENGTH)
#define max(a, b) ((a) > (b)) ? (a) : (b)
#define min(a, b) ((a) < (b)) ? (a) : (b)

struct packet_node {
	uint32_t seq;
	uint32_t len;
	struct packet_node *prev;
	struct packet_node *next;
	unsigned char data[MTU];
};

#endif
