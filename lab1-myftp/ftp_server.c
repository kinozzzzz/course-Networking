#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "myftp.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

void deal(int* argument)
{
    int client = *(argument);
    struct myftp_header header;
    while(1)
    {
        recv(client,&header,HEADER_LENGTH,0);
        byte type;
        unsigned int length;
        dec_header(&header,&type,NULL,&length);
        if(type == OPEN_CONN_REQUEST)
        {
            set_header(&header,OPEN_CONN_REPLY,1,HEADER_LENGTH);
            send(client,&header,HEADER_LENGTH,0);
            continue;
        }
        if(type == AUTH_REQUEST)
        {
            char buf[32];
            recv(client,buf,length-HEADER_LENGTH,0);
            if(memcmp(buf,"user 123123",12) == 0)
            {
                set_header(&header,AUTH_REPLY,1,HEADER_LENGTH);
                send(client,&header,HEADER_LENGTH,0);
            }
            else
            {
                set_header(&header,AUTH_REPLY,0,HEADER_LENGTH);
                send(client,&header,HEADER_LENGTH,0);
                sleep(10);
                close(client);
                break;
            }
        }
        if(type == LIST_REQUEST)
        {
            FILE * fp;
            char ls_answer[2048] = {0};
            char buf[64] = {0};
            int length = 0;
            fp = popen("ls","r");
            length = fread(ls_answer,1,2048,fp);
            ls_answer[length] = 0;
            struct myftp_header header;
            set_header(&header,LIST_REPLY,0,HEADER_LENGTH+length+1);
            send(client,&header,HEADER_LENGTH,0);
            send(client,ls_answer,length+1,0);
            continue;
        }
        if(type == GET_REQUEST)
        {
            char filename[16];
            recv(client,filename,length-HEADER_LENGTH,0);
            if(access(filename,F_OK) == 0)
            {
                set_header(&header,GET_REPLY,1,HEADER_LENGTH);
                send(client,&header,HEADER_LENGTH,0);
                struct stat statbuf;
                stat(filename,&statbuf);
                set_header(&header,FILE_DATA,0,HEADER_LENGTH+statbuf.st_size);
                send(client,&header,HEADER_LENGTH,0);
                unsigned char temp[1024];
                FILE *fp = fopen(filename,"rb");
                int length = 0;
                while(1)
                {
                    length = fread(temp,sizeof(char),1024,fp);
                    if(length == 0) break;
                    send(client,temp,length,0);
                }
                fclose(fp);
            }
            else
            {
                set_header(&header,GET_REPLY,0,HEADER_LENGTH);
                send(client,&header,HEADER_LENGTH,0);
            }
        }
        if(type == PUT_REQUEST)
        {
            char filename[16];
            recv(client,filename,length-HEADER_LENGTH,0);
            set_header(&header,PUT_REPLY,0,HEADER_LENGTH);
            send(client,&header,HEADER_LENGTH,0);
            FILE *fp = fopen(filename,"wb");
            char temp[1024] = {0};
            int length = 0;
            recv(client,&header,HEADER_LENGTH,0);
            dec_header(&header,NULL,NULL,&length);
            length -= HEADER_LENGTH;
            while(1)
            {
                recv(client,temp,min(1024,length),0);
                fwrite(temp,sizeof(char),min(1024,length),fp);
                length -= 1024;
                if(length <= 0) break;
            }
            fclose(fp);
        }
        if(type == QUIT_REQUEST)
        {
            set_header(&header,QUIT_REPLY,0,HEADER_LENGTH);
            send(client,&header,HEADER_LENGTH,0);
            sleep(10);
            close(client);
            break;
        }
    }
}
int main(int argc, char ** argv) {
    if(argc != 3)
    {
        printf("Usage: ftp_server [ip] [port],please retry!\n");
        exit(0);
    }
    int sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_pton(AF_INET,argv[1],&addr.sin_addr);
    addr.sin_port = htons(atoi(argv[2]));
    bind(sock,(struct sockaddr*)&addr,sizeof(addr));
    listen(sock,128);
    int client;
    while(1)
    {
        client = accept(sock,NULL,NULL);

        pthread_t thread;
        pthread_create(&thread,NULL,(void*)&deal,&client);
    }
}