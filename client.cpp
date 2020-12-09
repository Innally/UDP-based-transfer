#include<iostream>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>
#include <arpa/inet.h>
#include"datagram.h"
#include<ctime>
#include<fstream>

#define SERVER_PORT 8888
#define BUFF_LEN 1024
#define SERVER_IP "127.0.0.1"

using namespace std;

int nNetTimeout=1000;//1秒，
/*
    client:
             socket-->sendto-->revcfrom-->close
*/

void udp_msg_sender(int fd,struct sockaddr* dst);//no connect
bool udp_connect(int fd,struct sockaddr* dst);
void sendfile(int fd);
int ifgetack(int fd);
int resend(char*,int);

int main(int argc,char*argv[])
{
    int client_fd;// in linux int=socket
    struct sockaddr_in ser_addr;

    struct timeval timeout = {3,0};  

    client_fd=socket(AF_INET,SOCK_DGRAM,0);//AF_INET:IPV4,SOCK_DGRAM:UDP
    setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));//set overtime

    if(client_fd<0)
    {
        printf("create socket failed!\n");
        return-1;
    }

    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    //ser_addr.sin_addr.s_addr=inet_addr(SERVER_IP);
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);//IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port=htons(SERVER_PORT);//端口号，需要网络序转换
    
    //udp_msg_sender(client_fd,(struct sockaddr*)&ser_addr);
    if(udp_connect(client_fd,(struct sockaddr*)&ser_addr)==false)//if connect fail 
            return 2;
    //cout<<"test"<<endl;
    sendfile(client_fd);
    close(client_fd);
    return 0;
 
}

void udp_msg_sender(int fd,struct sockaddr* dst)
/*how to deal with */
{
    socklen_t len;
    struct  sockaddr_in src;//client_addr is senders' address
    while(1)
    {
        char buf[BUFF_LEN];
        cin.getline(buf,BUFF_LEN);
        printf("client:%s\n",buf);  //打印自己发送的信息
        len=sizeof(*dst);
        sendto(fd,buf,BUFF_LEN,0,dst,len);
        memset(buf,0,BUFF_LEN);//init buf
        recvfrom(fd,buf,BUFF_LEN,0,(struct sockaddr*)&src,&len);
        printf("server:%s\n",buf);
        sleep(1);// send every second
    }
    
}

bool udp_connect(int fd, struct sockaddr* to )
{
    char buf[10] = "200";
    int n = 0;
    int reconnect;

    //if(connect(fd, to, sizeof(*to)))// reconnect try 5 times.
    {
        for(reconnect=0;reconnect<5;reconnect++)
        {
            if(connect(fd, to, sizeof(*to))==0)
            {
                send(fd,buf,BUFF_LEN,0);
                memset(buf,0,10);
                int count=recv(fd,buf,BUFF_LEN,0);
                //cout<<count;
                if (count==-1)
                {
                    cout<<"connection not established."<<endl;
                    return 0;
                }
                cout<<buf<<endl;
                if(!strcmp(buf,"201"))
                {
                    cout<<"connection established."<<endl;
                    return 1;
                }
            }
        }
    }
    //else return;// success
}

void sendfile(int fd)
{
    char buf[BUFF_LEN];
    ifstream file(("helloworld.txt"),ios::binary);
    file.seekg( 0, ios::end );
	int filesize = file.tellg();
    file.seekg(0,ios::beg);
    if(file.is_open())
    {
        cout<<"file successfully oepn."<<endl;
    }
    else {
        cout<<"fail to read file";
        return;
    }
    int sendsize;
    int order=0;
    while(!file.eof())
    {
        sendsize=512;
        //cout<<"test2";
        //sleep(2);
        memset(buf,0,1024);
        cout<<filesize<<" "<<file.tellg()<<endl;
        //sleep(2);
        if(filesize-file.tellg()>=512)
            file.read(buf,512);                      // read
        else{
            sendsize=filesize-file.tellg()+1;
            file.read(buf,sendsize);
            break;
        }
        datagram message(fd,order,buf,sendsize);//datagram get read in message.d
        message.cal_checksum((unsigned short*)&message,
                             sizeof((unsigned short*)&message));
        memset(buf,0,1024);
        memcpy(buf,&message,sizeof(message));   // buf get message
        //cout<<message.d;
        send(fd,buf,1024,0);
        if(!ifgetack(fd))//if send fail, RESEND
            if(!resend(buf,fd))//if resend fail,print message;
                cout<<"resend still fail"<<endl;

        order++;
    }
    cout<<"all done"<<endl;
    file.close();
    cout<<"test"<<endl;
}

int ifgetack(int fd)
{
    char buf[10] ;
    memset(buf,0,10);
    recv(fd,buf,BUFF_LEN,0);
    //cout<<buf<<endl;
    if(!strcmp(buf,"ack"))
        return 1;
    else return 0;
}

int resend(char*buf,int fd)
{
    for(int i=0;i<5;i++)
    {
        send(fd,buf,BUFF_LEN,0);
        if(!ifgetack(fd))
            continue;
        else {
            return 1;//success
        }
    }
    return 0;//fail
}