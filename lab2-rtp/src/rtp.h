#ifndef RTP_H
#define RTP_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTP_START 0
#define RTP_END   1
#define RTP_DATA  2
#define RTP_ACK   3

#define PAYLOAD_SIZE 1461
#define MAX_SIZE 1472
#define SEQ_SPACE 1024
#define MIN(a,b) (a<=b ? a:b)
#define HEADER_LEN 11
#define LEN(a) (a+HEADER_LEN)


typedef struct __attribute__ ((__packed__)) RTP_header {
    uint8_t type;       // 0: START; 1: END; 2: DATA; 3: ACK
    uint16_t length;    // Length of data; 0 for ACK, START and END packets
    uint32_t seq_num;
    uint32_t checksum;  // 32-bit CRC
} rtp_header_t;


typedef struct __attribute__ ((__packed__)) RTP_packet {
    rtp_header_t header;
    char payload[PAYLOAD_SIZE];
} rtp_packet_t;

void set_packet(rtp_packet_t *pkt,uint8_t type,uint16_t length,uint32_t seq_num);

struct rtp_inf
{
    int sock_fd;
    uint32_t window_size;
    struct sockaddr_in addr;
    uint32_t seq_num;
};

int timerecv(int sock_fd,rtp_packet_t *pkt,uint16_t size,clock_t wait_time);

int checksum_pkt(rtp_packet_t *pkt);

void print_pkt(rtp_packet_t *pkt);

void send_header(uint8_t type,uint32_t seq_num,int sock_fd,struct sockaddr_in addr);

#ifdef __cplusplus
}
#endif

#endif //RTP_H
