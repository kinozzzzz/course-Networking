#include "rtp.h"
#include "util.h"

void set_packet(rtp_packet_t *pkt,uint8_t type,uint16_t length,uint32_t seq_num)
{
    (pkt->header).type = type;
    (pkt->header).length = length;
    (pkt->header).seq_num = seq_num;
    (pkt->header).checksum = 0;
    (pkt->header).checksum = compute_checksum(pkt,LEN(length));
    //printf("-----from set_packet\n");
    //print_pkt(pkt);
}

/*
return 0 if received
return -1 if not
*/
int timerecv(int sock_fd,rtp_packet_t *pkt,uint16_t size,clock_t wait_time)
{
    clock_t start_time = clock();
    while(1)
    {
        //int len = recvfrom(sock_fd,pkt,size,0,addr,addr_len);
        if(recvfrom(sock_fd,pkt,size,0,NULL,NULL) < 0)
        {
            clock_t now_time = clock();
            if(now_time - start_time >= wait_time)
            {
                return -1;
            }
            continue;
        }
        return 0;
    }
}

/*
return 0 if correct
return 1 if wrong
*/
int checksum_pkt(rtp_packet_t *pkt)
{
    uint32_t checksum = pkt->header.checksum;
    pkt->header.checksum = 0;
    if(compute_checksum(pkt,LEN((pkt->header).length)) == checksum)
        return 0;
    else
    {
        // printf("checksum in pkt: %x\n",checksum);
        // printf("checksum calculated: %x\n",compute_checksum(pkt,LEN(pkt->header.length)));
        return 1;
    }
}

/*
For debug
*/
void print_pkt(rtp_packet_t *pkt)
{
    if((pkt->header).type == RTP_ACK)
        printf("Type: RTP_ACK\n");
    else if((pkt->header).type == RTP_END)
        printf("Type: RTP_END\n");
    else if(pkt->header.type == RTP_DATA)
        printf("Type: RTP_DATA\n");
    else
        printf("Type: RTP_START\n");
    printf("Length: %d\n",pkt->header.length);
    printf("Seq_num: %d\n",pkt->header.seq_num);
    printf("Checksum: %x\n\n",pkt->header.checksum);
}

/*
just send a header
*/
void send_header(uint8_t type,uint32_t seq_num,int sock_fd,struct sockaddr_in addr)
{
    rtp_header_t header;
    bzero(&header,sizeof(header));
    set_packet((rtp_packet_t *)&header,type,0,seq_num);
    int len = sendto(sock_fd,&header,HEADER_LEN,0,(struct sockaddr*)&addr,sizeof(addr));
    // printf("len: %d\n",len);
    // print_pkt((rtp_packet_t*)&header);
}