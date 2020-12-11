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

//---------------------------------------------------------
//connect make sure to be connected
bool udp_connect(int fd,struct sockaddr* dst);
//send file, called if_getack(),and called resend()
void sendfile(int fd,const char* filename);
//make sure server recived
int ifgetack(int fd);
//when one time didn't get ack, then resent
int resend(char*,int);
//---------------------------------------------------------


int main(int argc,char*argv[])
{
    //----------------------------------------init-------------------------------------------------
    int client_fd;                         // in linux int=socket
    struct sockaddr_in ser_addr;           //server address
    struct timeval timeout = {3,0};        //to set a rev waiting time

    setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));//set overtime
    client_fd=socket(AF_INET,SOCK_DGRAM,0);//AF_INET:IPV4,SOCK_DGRAM:UDP
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
    //----------------------------------------------------------------------------------------------


    //--------------------------------------connect---------------------------------------------------
    if(udp_connect(client_fd,(struct sockaddr*)&ser_addr)==false)//if connect fail 
            return 2;
    //cout<<"test"<<endl;
    //--------------------------------------sendfile)--------------------------------------------------
    const char * filename{"helloworld.txt"};
    sendfile(client_fd,filename);

    //-------------------------------------------------------------------------------------------------
    close(client_fd);
    return 0;
 
}


bool udp_connect(int fd, struct sockaddr* to )
{
    char buf[10] = "200"; 

    //----------------------reconnect try 5 times.--------------------------------
    {
        for(int reconnect=0;reconnect<5;reconnect++)
        {
            if(connect(fd, to, sizeof(*to))==0)//connect successfully
            {
                send(fd,buf,BUFF_LEN,0);       // send to server 200
                memset(buf,0,10);
                int count=recv(fd,buf,BUFF_LEN,0);
                //cout<<count;
                if (count==-1)         // can't get 201 from server
                {
                    cout<<"connection not established."<<endl;
                    return 0;
                }
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

void sendfile(int fd,const char* c)
{
    char buf[BUFF_LEN];
    int sendsize;
    int order=0;
    //-------------------------open file--------------------------------
    ifstream file(c,ios::binary);
    file.seekg( 0, ios::end );
	int filesize = file.tellg();//get how big the file is.
    file.seekg(0,ios::beg);
    if(file.is_open())
    {
        cout<<"file successfully oepn."<<endl;
    }
    else {
        cout<<"fail to read file";
        return;
    }
    //---------------------------------------------------------------------

    // read and send the file block
    while(!file.eof())
    {
        sendsize=512;
        memset(buf,0,1024);
        sleep(1);
        //-------------------------read blocks----------------------------------
        if(filesize-file.tellg()>=512)
            //normally send 512
            file.read(buf,512);                      // read
        else{
            //when there's no enough data to form a 512 byte data
            sendsize=filesize-file.tellg()+1;
            file.read(buf,sendsize);
            
        }
        //------------------------------------------------------------------------

        //---------fileblock-->buf-->message.d--->memset(buf)-->message-->buf-->send(buf)---------------
        datagram message(fd,order,buf,sendsize);//datagram get read in message.d
        memset(buf,0,1024);
        memcpy(buf,&message,sizeof(message));   // buf get message
        message.checksum=message.cal_checksum((unsigned short*)message.d,sizeof(message.d)/sizeof(unsigned short));
        //cout<<message.checksum<<endl;
        send(fd,buf,1024,0);
        if (!ifgetack(fd))
            resend(buf,fd);

        //-----------------------mistake or lost check----------------------------
        if(!ifgetack(fd))//if send fail, RESEND
            if(!resend(buf,fd))//if resend fail,print message;
                cout<<"resend still fail"<<endl;
        //------------------------------------------------------------------------
        order++;
    }
    cout<<"all done"<<endl;
    file.close();
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
    //--------------try resend the message for 5 times
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