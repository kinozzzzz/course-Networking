#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "myftp.h"
#include <unistd.h>
#include <sys/stat.h>

int get_string(char * buf)
{
    char c = getchar();
    while(c == ' ' || c == '\n'){continue;}
    if(c == EOF) return 0;
    int i = 0;
    while(c != ' ' && c != '\n')
    {
        buf[i++] = c;
        c = getchar();
    }
    buf[i] = '\0';
    //printf("buf: %s\n",buf);
    return i;
}
int main(int argc, char ** argv) {
    char buf[32] = {0};
    int server;
    int esta_conc = 0;  //是否和server建立连接的flag
    int auth_passed = 0;     //是否通过身份认证
    while(printf("client> ") && get_string(buf))
    {
        if(memcmp(buf,"open",4) == 0 && esta_conc == 0)
        {
            struct sockaddr_in addr;
            server = socket(AF_INET,SOCK_STREAM,0);

            addr.sin_family = AF_INET;
            memset(buf,0,sizeof(buf));
            get_string(buf);
            inet_pton(AF_INET,buf,&addr.sin_addr);
            memset(buf,0,sizeof(buf));
            get_string(buf);
            addr.sin_port = htons(atoi(buf));
            if(connect(server,(struct sockaddr*)&addr,sizeof(addr)) < 0)
            {
                printf("Socket failed,please check your input!\n");
            }
            else
            {
                struct myftp_header header;
                set_header(&header,OPEN_CONN_REQUEST,0,HEADER_LENGTH);
                send(server,&header,HEADER_LENGTH,0);
                while(recv(server,&header,HEADER_LENGTH,0))
                {
                    if(check_header(&header) && header.m_type == OPEN_CONN_REPLY)
                        break;
                }
                byte status;
                dec_header(&header,NULL,&status,NULL);
                if(status == 1)
                {
                    esta_conc = 1;  //建立连接成功，修改flag为1
                    printf("The connection is established successfully.\n");
                }
                else
                {
                    printf("Can't establish the connection with server\n");
                }                
            }
            continue;
        }
        if(!esta_conc)
        {
            printf("please establish connection first!\n");
            continue;
            //直到和服务器建立连接之前不执行其他命令
        }
        if(memcmp(buf,"auth",4) == 0 && auth_passed == 0)
        {
            char user[16] = {0};
            char pass[16] = {0};
            char payload[32] = {0};
            get_string(user);
            get_string(pass);
            memcpy(payload,user,strlen(user));
            payload[strlen(user)] = ' ';
            memcpy(payload+strlen(user)+1,pass,strlen(pass));
            struct myftp_header header;
            set_header(&header,AUTH_REQUEST,0,HEADER_LENGTH+strlen(payload)+1);
            send(server,&header,HEADER_LENGTH,0);
            send(server,payload,strlen(payload)+1,0);
            while(recv(server,&header,HEADER_LENGTH,0))
            {
                if(check_header(&header) && header.m_type == AUTH_REPLY)
                    break;
            }
            byte status;
            dec_header(&header,NULL,&status,NULL);
            if(status == 1)
            {
                printf("Authentication passed!\n");
                auth_passed = 1;
                continue;
            }
            else
            {
                printf("Authentication failed!Connection closed!\n");
                close(server);
                esta_conc = 0;
                continue;
            }
        }
        if (!auth_passed)
        {
            printf("Sorry!You should pass the authentication!\n");
            continue;
            //必须通过身份认证，才可以进行其他操作
        }
        if(memcmp(buf,"ls",2) == 0)
        {
            struct myftp_header header;
            set_header(&header,LIST_REQUEST,0,HEADER_LENGTH);
            send(server,&header,HEADER_LENGTH,0);
            recv(server,&header,HEADER_LENGTH,0);
            unsigned int length = 0;
            dec_header(&header,NULL,NULL,&length);
            char * payload = malloc(length-HEADER_LENGTH);
            recv(server,payload,length-HEADER_LENGTH,0);
            printf("------------------list answer-----------------\n");
            printf("%s",payload);
            free(payload);
        }
        if(memcmp(buf,"get",3) == 0)
        {
            get_string(buf);
            struct myftp_header header;
            set_header(&header,GET_REQUEST,0,HEADER_LENGTH+strlen(buf)+1);
            send(server,&header,HEADER_LENGTH,0);
            send(server,buf,strlen(buf)+1,0);
            recv(server,&header,HEADER_LENGTH,0);
            byte status;
            dec_header(&header,NULL,&status,NULL);
            if(status)
            {
                FILE *fp = fopen(buf,"wb");
                char temp[1024] = {0};
                int length = 0;
                recv(server,&header,HEADER_LENGTH,0);
                dec_header(&header,NULL,NULL,&length);
                length -= HEADER_LENGTH;
                printf("Please wait seconds...\n");
                while(1)
                {
                    recv(server,temp,min(1024,length),0);
                    fwrite(temp,sizeof(char),min(1024,length),fp);
                    length -= 1024;
                    if(length <= 0) break;
                }
                fclose(fp);
                printf("Get successfully!\n");
            }
            else
                printf("File is not on the server!\n");
        }
        if(memcmp(buf,"put",3) == 0)
        {
            get_string(buf);
            struct myftp_header header;
            if(access(buf,F_OK) == 0)
            {
                struct stat statbuf;
                stat(buf,&statbuf);
                set_header(&header,PUT_REQUEST,0,HEADER_LENGTH+strlen(buf)+1);
                send(server,&header,HEADER_LENGTH,0);
                send(server,buf,strlen(buf)+1,0);
                recv(server,&header,HEADER_LENGTH,0);
                set_header(&header,FILE_DATA,0,HEADER_LENGTH+statbuf.st_size);
                send(server,&header,HEADER_LENGTH,0);
                unsigned char temp[1024];
                FILE *fp = fopen(buf,"rb");
                int length = 0;
                printf("Please wait seconds!\n");
                while(1)
                {
                    length = fread(temp,sizeof(char),1024,fp);
                    if(length == 0) break;
                    send(server,temp,length,0);
                }
                printf("Put successfully!\n");
                fclose(fp);
            }
            else
                printf("Not found the file,please check the filepath!\n");
        }
        if(memcmp(buf,"quit",4) == 0)
        {
            struct myftp_header header;
            set_header(&header,QUIT_REQUEST,0,HEADER_LENGTH);
            send(server,&header,HEADER_LENGTH,0);
            if(recv(server,&header,HEADER_LENGTH,0))
            {
                if(header.m_type == QUIT_REPLY)
                {
                    close(server);
                    printf("Quit successfully!\n");
                    exit(0);
                }
            }
            

        }
    }
    return 0;
}