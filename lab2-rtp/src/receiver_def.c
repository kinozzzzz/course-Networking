#include "receiver_def.h"
#include "rtp.h"
#include "util.h"

static struct rtp_inf inf_r;

int initReceiver(uint16_t port, uint32_t window_size)
{
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd < 0)
    {
        printf("<socket> failed\n");
        return -1;
    }
    struct sockaddr_in local_addr;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    if(bind(sock_fd,(struct sockaddr*)&local_addr,sizeof(local_addr))  < 0)
    {
        printf("<bind> failed\n");
        return -1;
    }
    
    rtp_packet_t recv_pkt;
    struct sockaddr_in send_addr;
    int len = sizeof(send_addr);
    bzero(&recv_pkt,MAX_SIZE);
    if(recvfrom(sock_fd,&recv_pkt,MAX_SIZE,0,(struct sockaddr*)&send_addr,&len) < 0)
    {
        printf("<recvfrom> failed\n");
        return -1;
    }
    if(checksum_pkt(&recv_pkt))
    {
        printf("checksum error\n");
        exit(-1);
    }
    send_header(RTP_ACK,recv_pkt.header.seq_num,sock_fd,send_addr);
    if(fcntl(sock_fd,F_SETFL,fcntl(sock_fd,F_GETFL,0) | O_NONBLOCK) < 0)
    {
        printf("Setting nonblock failed\n");
        return -1;
    }
    inf_r.sock_fd = sock_fd;
    inf_r.window_size = window_size;
    inf_r.addr = send_addr;
    return 0;
}

int recvMessage(char* filename)
{  
    inf_r.seq_num = 0;
    FILE *fp = fopen(filename,"wb+");
    rtp_packet_t recv_pkt;
    uint32_t recvbytes = 0;
    rtp_packet_t pkts[inf_r.window_size];
    bzero(pkts,MAX_SIZE * inf_r.window_size);
    while(1)
    {
        if(timerecv(inf_r.sock_fd,&recv_pkt,MAX_SIZE,10000000) < 0)
        {
            printf("TIME OUT!\n");
            return recvbytes;
        }
        // printf("-------from sender  seq_num: %d\n",inf_r.seq_num);
        // print_pkt(&recv_pkt);
        if(checksum_pkt(&recv_pkt))
        {
            continue;
        }
        if(recv_pkt.header.type == RTP_END)
        {
            return recvbytes;
        }
        uint32_t seq_num = recv_pkt.header.seq_num;
        if(seq_num < inf_r.seq_num)
            continue;
        if(seq_num >= inf_r.seq_num+inf_r.window_size)
            continue;
        if(seq_num == inf_r.seq_num)
        {
            fwrite(recv_pkt.payload,1,recv_pkt.header.length,fp);
            int i;
            for(i=inf_r.seq_num+1;i < inf_r.seq_num+inf_r.window_size;i++)
            {
                rtp_packet_t *temp = pkts + (i % inf_r.window_size);
                if(temp->header.type == RTP_DATA)
                {
                    fwrite(temp->payload,1,(temp->header).length,fp);
                    recvbytes += (temp->header).length;
                    bzero(temp,MAX_SIZE);
                }
                else
                {
                    break;
                }
            }
            inf_r.seq_num = i;
            fflush(fp);
        }
        else
        {
            memcpy(pkts+(seq_num % inf_r.window_size),&recv_pkt,MAX_SIZE);
        }
        send_header(RTP_ACK,inf_r.seq_num,inf_r.sock_fd,inf_r.addr);
    }
}

void terminateReceiver()
{
    printf("Receiver terminated\n");
    send_header(RTP_ACK,inf_r.seq_num,inf_r.sock_fd,inf_r.addr);
    bzero(&inf_r,sizeof(inf_r));
}

int recvMessageOpt(char* filename)
{
    inf_r.seq_num = 0;
    FILE *fp = fopen(filename,"wb+");
    rtp_packet_t recv_pkt;
    uint32_t recvbytes = 0;
    rtp_packet_t pkts[inf_r.window_size];
    bzero(pkts,MAX_SIZE * inf_r.window_size);
    while(1)
    {
        if(timerecv(inf_r.sock_fd,&recv_pkt,MAX_SIZE,10000000) < 0)
        {
            printf("TIME OUT!\n");
            return recvbytes;
        }
        // printf("-------from sender  seq_num: %d\n",inf_r.seq_num);
        // print_pkt(&recv_pkt);
        if(checksum_pkt(&recv_pkt))
        {
            continue;
        }
        if(recv_pkt.header.type == RTP_END)
        {
            return recvbytes;
        }
        uint32_t seq_num = recv_pkt.header.seq_num;
        if(seq_num < inf_r.seq_num)
        {
            send_header(RTP_ACK,seq_num,inf_r.sock_fd,inf_r.addr);
            continue;
        }
        if(seq_num >= inf_r.seq_num+inf_r.window_size)
            continue;
        if(seq_num == inf_r.seq_num)
        {
            fwrite(recv_pkt.payload,1,recv_pkt.header.length,fp);
            int i;
            for(i=inf_r.seq_num+1;i < inf_r.seq_num+inf_r.window_size;i++)
            {
                rtp_packet_t *temp = pkts + (i % inf_r.window_size);
                if(temp->header.type == RTP_DATA)
                {
                    fwrite(temp->payload,1,(temp->header).length,fp);
                    recvbytes += (temp->header).length;
                    bzero(temp,MAX_SIZE);
                }
                else
                {
                    break;
                }
            }
            inf_r.seq_num = i;
            fflush(fp);
        }
        else
        {
            memcpy(pkts+(seq_num % inf_r.window_size),&recv_pkt,MAX_SIZE);
        }
        send_header(RTP_ACK,seq_num,inf_r.sock_fd,inf_r.addr);
    }
}

// int main()
// {
//     if(initReceiver(12346,32) == 0)
//     {
//         printf("init succeed\n");
//         recvMessage("test.txt");
//         terminateReceiver();
//     }
// }