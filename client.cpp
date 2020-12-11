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
#include <vector>
#include<time.h>

#define SERVER_PORT 8888
#define BUFF_LEN 1024
#define SERVER_IP "127.0.0.1"
#define WINDOW 5
using namespace std;


int flag=0;
const char * filename{"helloworld.txt"};
ifstream file(filename,ios::binary);

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
int ifgetack(int fd,int& ack);
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
    
    sendfile(client_fd,filename);
    cout<<"all done"<<endl;
    file.close();

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
    vector<datagram> mesarray;
    char buf[BUFF_LEN];
    int sendsize=512;
    int order=0;
    //-------------------------open file--------------------------------
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
    int right=4;
    int left=0;
    int ack=0;
    int addslide=0;
    for(int i=0;i<5;i++)
    {
        file.read(buf,512);   
        datagram message(fd,order,buf,sendsize);//datagram get read in message.d
        memcpy(message.d,buf,sizeof(message.d));
        mesarray.push_back(message);
        order++;
    }
    while(file.peek())
    {
    
        sendsize=512;
        memset(buf,0,1024);
        //-------------------------read blocks----------------------------------
        
        printf("right is %d,ack is %d,addslide is %d ",right,ack,addslide);
        for(int i=0;i<addslide;i++)//read how many block: 5-(block number has ack)
        {
            if(filesize-file.tellg()>=512)
                {
                    //normally send 512
                    file.read(buf,512);
                    
                    datagram message(fd,order,buf,sendsize);//datagram get read in message.d
                    message.cal_checksum();
                    mesarray.push_back(message);
                    //cout<<"message "<<message.order<<endl;
                    //cout<<"test"<<endl;
                    order++;
                }                      // read
            else{
                //when there's no enough data to form a 512 byte data
                sendsize=filesize-file.tellg();
                file.read(buf,sendsize);
                datagram message(fd,order,buf,sendsize);//datagram get read in message.d
                message.cal_checksum();
                mesarray.push_back(message);
                cout<<"the last pack:"<<message.order<<endl;
                flag=i;;
                break;
                
                }
        }
        //------------------------------------------------------------------------
        //---------fileblock-->buf-->message.d--->memset(buf)-->message-->buf-->send(buf)---------------
        for (int i=0;i<WINDOW;i++)
        {
            memset(buf,0,1024);
            memcpy(buf,&mesarray[i],sizeof(mesarray[i]));   // buf get message
            send(fd,buf,1024,0);
            if(flag!=0 && flag==i)
            {
                    return;
            }

        }

        //-----------------------mistake or lost check----------------------------
        ifgetack(fd,ack);
        //------------------------------------------------------------------------
        addslide=ack-right+5;
        mesarray.erase(mesarray.begin(),mesarray.begin()+addslide);
        right=ack+5;

        //mesarray.clear();
    }
}

int ifgetack(int fd,int& ack)
{
    char buf[sizeof(int)] ;
    memset(buf,0,sizeof(int));
    recv(fd,buf,sizeof(int),0);
    memcpy(&ack,buf,sizeof(int));
    cout<<"recieved ack:"<<ack<<endl;
    return 0;
}

// int resend(char*buf,int fd)
// {
//     //--------------try resend the message for 5 times
//     for(int i=0;i<5;i++)
//     {
//         send(fd,buf,BUFF_LEN,0);
//         if(!ifgetack(fd))
//             continue;
//         else {
//             return 1;//success
//         }
//     }
//     return 0;//fail
// }