#include "sender_def.h"
#include "rtp.h"
#include "util.h"

static struct rtp_inf inf_s;
//用于标识文件是否已经读取完毕以及最后要发的packet的seq_num
int end_seq = 0;

void ReadFromFile(FILE *fp,rtp_packet_t *pkt,uint32_t seq_num)
{
    if(end_seq)
        return;
    int length = fread(pkt->payload,1,PAYLOAD_SIZE,fp);
    set_packet(pkt,RTP_DATA,length,seq_num);
    if(length == 0)
        end_seq = seq_num - 1;
    if(length < PAYLOAD_SIZE)
        end_seq = seq_num;
    //print_pkt(pkt);
}

/*
return -1 if time out
return 0 if succeed
*/
int SendAndRecv(uint8_t type,rtp_packet_t *recv_pkt)
{
    bzero(recv_pkt,MAX_SIZE);
    send_header(type,inf_s.seq_num,inf_s.sock_fd,inf_s.addr);
    if(timerecv(inf_s.sock_fd,recv_pkt,MAX_SIZE,10*TIMEOUT) < 0)
    {
        printf("<timerecv> time out\n");
        return -1;
    }
    return 0;
}

//把seq_num从start到end的之间的packet全部发出去
void Send(int start,int end,rtp_packet_t *pkts)
{
    rtp_packet_t *temp;
    for(int i=start;i < end;i++)
    {
        if(end_seq != 0 && i > end_seq)
            return;
        temp = pkts + (i%inf_s.window_size);
        if(temp->header.type != RTP_DATA)
            continue;
        if(sendto(inf_s.sock_fd,temp,LEN(temp->header.length),0,(struct sockaddr *)&inf_s.addr,sizeof(inf_s.addr)) < 0)
        {
            printf("send failed\n");
            continue;
        }
        // printf("start: %d end: %d seq_num: %d\n",start,end,inf_s.seq_num);
        // print_pkt(temp);
    }
}

//将当前窗口中未接受到的报文全部重新发送
void AllSend(rtp_packet_t *pkts)
{
    Send(inf_s.seq_num,inf_s.seq_num+inf_s.window_size,pkts);
}

int initSender(const char* receiver_ip, uint16_t receiver_port, uint32_t window_size)
{
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd < 0)
    {
        printf("<socket> failed\n");
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET,receiver_ip,&addr.sin_addr.s_addr);
    addr.sin_port = htons(receiver_port);
    if(fcntl(sock_fd,F_SETFL,fcntl(sock_fd,F_GETFL,0) | O_NONBLOCK) < 0)
    {
        printf("Setting nonblock failed\n");
        return -1;
    }
    inf_s.sock_fd = sock_fd;
    inf_s.window_size = window_size;
    inf_s.addr = addr;

    rtp_packet_t recv_pkt;
    uint32_t seq_num = rand();
    inf_s.seq_num = seq_num;
    if(SendAndRecv(RTP_START,&recv_pkt) < 0)
    {
        //接受ACK报文超时，发送END报文
        terminateSender();
        return -1;
    }
    if(checksum_pkt(&recv_pkt))
    {
        //checksum不对，直接发送END报文
        printf("checksum error----------------\n");
        print_pkt(&recv_pkt);
        printf("------------------------------\n");
        terminateSender();
        return -1;
    }
    if(!(recv_pkt.header.type == RTP_ACK && recv_pkt.header.seq_num == seq_num))
    {
        printf("Get the wrong received pkt\n");
        print_pkt(&recv_pkt);
        terminateSender();
        return -1;
    }
    return 0;
}

int sendMessage(const char* message)
{
    inf_s.seq_num = 0;
    FILE * fp = fopen(message,"rb+");
    if(fp < 0)
    {
        printf("Opening file failed\n");
        return -1;
    }
    rtp_packet_t pkts[inf_s.window_size];
    for(int i=0;i < inf_s.window_size;i++)
    {
        ReadFromFile(fp,pkts+i,i);
    }
    AllSend(pkts);
    rtp_packet_t recv_pkt;
    while(1)
    {
        if(end_seq != 0 && inf_s.seq_num > end_seq)
        {
            printf("Sending completed\n");
            return 0;
        }
        if(timerecv(inf_s.sock_fd,&recv_pkt,MAX_SIZE,TIMEOUT) < 0)
        {
            //printf("TIMEOUT! Resend\n");
            AllSend(pkts);
            continue;
        }
        if(checksum_pkt(&recv_pkt))
        {
            //校验和不对
            continue;
        }
        // printf("-------from recv\n");
        // print_pkt(&recv_pkt);
        if(recv_pkt.header.seq_num <= inf_s.seq_num)
            continue;
        if(recv_pkt.header.seq_num > inf_s.seq_num + inf_s.window_size)
            continue;
        for(int i=inf_s.seq_num;i < recv_pkt.header.seq_num;i++)
        {
            rtp_packet_t *temp = pkts+(i%inf_s.window_size);
            ReadFromFile(fp,temp,i+inf_s.window_size);
        }
        Send(inf_s.seq_num,recv_pkt.header.seq_num,pkts);
        inf_s.seq_num = recv_pkt.header.seq_num;
    }
    return 0;
}

void terminateSender()
{
    printf("Sender terminated\n");
    rtp_packet_t recv_pkt;
    SendAndRecv(RTP_END,&recv_pkt);
    
    close(inf_s.sock_fd);
    bzero(&inf_s,sizeof(inf_s));
}

int sendMessageOpt(const char* message)
{
    inf_s.seq_num = 0;
    FILE * fp = fopen(message,"rb+");
    if(fp < 0)
    {
        printf("Opening file failed\n");
        return -1;
    }
    rtp_packet_t pkts[inf_s.window_size];
    for(int i=0;i < inf_s.window_size;i++)
    {
        ReadFromFile(fp,pkts+i,i);
    }
    AllSend(pkts);
    rtp_packet_t recv_pkt;
    while(1)
    {
        if(end_seq != 0 && inf_s.seq_num > end_seq)
        {
            printf("Sending completed\n");
            return 0;
        }
        if(timerecv(inf_s.sock_fd,&recv_pkt,MAX_SIZE,TIMEOUT) < 0)
        {
            //printf("TIMEOUT! Resend\n");
            AllSend(pkts);
            continue;
        }
        if(checksum_pkt(&recv_pkt))
        {
            //校验和不对
            continue;
        }
        // printf("-------OPT from recv\n");
        // print_pkt(&recv_pkt);
        uint32_t seq_num = recv_pkt.header.seq_num;
        if(seq_num < inf_s.seq_num)
            continue;
        if(seq_num >= inf_s.seq_num + inf_s.window_size)
            continue;
        if(seq_num == inf_s.seq_num)
        {
            int i;
            for(i = inf_s.seq_num+1;i < inf_s.seq_num+inf_s.window_size;i++)
            {
                rtp_packet_t *temp = pkts + (i % inf_s.window_size);
                //通过检验type,从而判断是否这个packet已经收到过对应的ACK
                if(temp->header.type == RTP_DATA)
                {
                    break;
                }
            }
            for(int j=inf_s.seq_num;j < i;j++)
            {
                rtp_packet_t *temp = pkts+(j%inf_s.window_size);
                ReadFromFile(fp,temp,j+inf_s.window_size);
            }
            Send(inf_s.seq_num,i,pkts);
            inf_s.seq_num = i;
        }
        else
        {
            //如果收到对应的ACK则把对应的pkt
            (pkts+(seq_num % inf_s.window_size))->header.type = 0;
        }
    }
    return 0;
}


// int main(int argc, char **argv) 
// {  
//     if(initSender("127.0.0.1", 12346, 32) == 0) 
//     {
//         printf("get into sender inner\n"); 
//         sendMessage("testdata"); 
//         terminateSender(); 
//     }
//     return 0;
// }