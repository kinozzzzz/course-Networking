#include <string.h>
#include <arpa/inet.h>

#define MAGIC_NUMBER_LENGTH 6
#define HEADER_LENGTH 12
#define OPEN_CONN_REQUEST 0xA1
#define OPEN_CONN_REPLY 0xA2
#define AUTH_REQUEST 0xA3
#define AUTH_REPLY 0xA4
#define LIST_REQUEST 0xA5
#define LIST_REPLY 0xA6
#define GET_REQUEST 0xA7
#define GET_REPLY 0xA8
#define FILE_DATA 0xFF
#define PUT_REQUEST 0xA9
#define PUT_REPLY 0xAA
#define QUIT_REQUEST 0xAB
#define QUIT_REPLY 0xAC
typedef unsigned char byte;
struct myftp_header
{
    byte m_protocol[MAGIC_NUMBER_LENGTH];   /* protocol magic number (6 bytes) */
    byte m_type;                            /* type (1 byte) */
    byte m_status;                          /* status (1 byte) */
    unsigned int m_length;                  /* length (4 bytes) in Big endian*/
} __attribute__ ((packed));

void set_header(struct myftp_header * header,byte type,byte status,unsigned int length)
{
    memcpy(header->m_protocol,"\xe3myftp",6);
    header->m_type = type;
    header->m_status = status;
    header->m_length = htonl(length);   //报文中的length以大端序存储
}

void dec_header(struct myftp_header * header,byte *type,byte *status,unsigned int *length)
{
    if(type)
        *type = header->m_type;
    if(status)
        *status = header->m_status;
    if(length)
        *length = ntohl(header->m_length);
}

int check_header(struct myftp_header * header)
{
    return !memcmp(header->m_protocol,"\xe3myftp",MAGIC_NUMBER_LENGTH);
}

int min(int a,int b)
{
    return a <= b ? a : b;
}