#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#define HEADER_LENGTH 4
#define MTU (1500 - 20 - 8 - HEADER_LENGTH)

struct packet_node {
	uint32_t seq;
	uint32_t len;
	struct packet_node *prev;
	struct packet_node *next;
	unsigned char data[MTU];
};

#endif
