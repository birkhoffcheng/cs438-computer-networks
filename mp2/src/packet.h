#ifndef PACKET_H
#define PACKET_H

#define HEADER_LENGTH 4
#define MTU 1500
#define MAX_PAYLOAD_SIZE (MTU - 20 - 8 - HEADER_LENGTH)
#define max(a, b) ((a) > (b)) ? (a) : (b)
#define min(a, b) ((a) < (b)) ? (a) : (b)

#endif
