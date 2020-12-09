#include<iostream>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>
#include"datagram.h"
#include<fstream>

#define SERVER_PORT 8888
#define BUFF_LEN 1024

using namespace std;

struct sockaddr_in client_addr;
socklen_t len= sizeof(client_addr);

/*
    server:
            socket-->bind-->read-->sendto-->close
*/
void udp_msg_process(int fd);
void udp_connect_to_client(int fd, struct sockaddr* to,struct sockaddr_in &client_addr);//fd selfsocket
void getfile(int);
int acknowledge(int);
    

int main(int argc,char*argv[])
{
    int server_fd, ret;// in linux int=socket
    struct sockaddr_in ser_addr;

    server_fd=socket(AF_INET,SOCK_DGRAM,0);//AF_INET:IPV4,SOCK_DGRAM:UDP
    if(server_fd<0)
    {
        printf("create socket failed!\n");
        return-1;
    }
   
    memset(&ser_addr,0,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_addr.s_addr=htonl(INADDR_ANY);//IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port=htons(SERVER_PORT);//端口号，需要网络序转换
    
    ret=bind(server_fd,(struct  sockaddr*)&ser_addr,sizeof(ser_addr));
    if(ret<0)
    {
        printf("socket bind failed\n");
        return-1;
    }
    //to connect
    udp_connect_to_client(server_fd,(struct  sockaddr*)&ser_addr,client_addr);
    //start transfer
    getfile(server_fd);
    close(server_fd);
    return 0;
 
}


void udp_connect_to_client(int fd, struct sockaddr* to,struct sockaddr_in &client_addr)
{
    //struct  sockaddr_in client_addr;//client_addr is senders' address
    char buf[10] ;
    memset(buf,0,10);
    //cout<<"test1"<<endl;
    cout<<"waiting for connection."<<endl;
    recvfrom(fd,buf,10,0,(struct sockaddr*)&client_addr,&len); //recvfrom是拥塞函,没有数据就一直拥塞
    //cout<<"test2"<<endl;
    if(!strcmp(buf,"200"))
    {
        memset(buf,0,10);
        strcpy(buf,"201");
        sendto(fd,buf,10,0,(struct sockaddr*)&client_addr,len);
        //recv(fd,buf,BUFF_LEN,0);
        cout<<endl;
    }

}

void getfile(int fd)
{
    char buf[BUFF_LEN];
    int order=0;    
    ofstream file("testtext.txt",ios::binary);
    if(!file.is_open())
    {
        cout<<"file not open"<<endl;
    }
    while(1)
    {
        memset(buf,0,1024);
        datagram message(fd,order,buf);
        int count=recv(fd, buf, BUFF_LEN, 0);
        acknowledge(fd);
        memcpy(&message,buf,sizeof(message));//!!!!
        file.write(message.d,message.sendsize);
        if (message.sendsize!=512)
            break;
        cout<<"get "<<count<<" bytes"<<endl;
        order++;
    }
    file.close();
}


int acknowledge(int fd)
{
    char buf[10] ;
    memset(buf,0,10);
    strcpy(buf,"ack");
    sendto(fd,buf,10,0,(struct sockaddr*)&client_addr,len);
}

// void udp_msg_process(int fd)
// /*how to deal with */
// {
//     //cout<<"in";
//     char buf[BUFF_LEN];//buffer,528byte
//     socklen_t len;
//     int count;
//     struct  sockaddr_in client_addr;//client_addr is senders' address
//     while(1)
//     {
//         memset(buf,0,BUFF_LEN);//init buf
//         len= sizeof(client_addr);
//         count=recvfrom(fd,buf,BUFF_LEN,0,(struct sockaddr*)&client_addr,&len); //recvfrom是拥塞函,没有数据就一直拥塞
//         struct sockaddr*to=(struct sockaddr*)&client_addr;
//         bool ifconnect=udp_connect_to_client(fd,to);
//         if(!ifconnect)//if connection fail, go back to waiting status
//         {
//             cout<<"connection can't be established.";
//             continue;
//         }
//         else{
//             getfile(fd);
//         }
 
//     }
   
// }