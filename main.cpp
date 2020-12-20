#include<iostream>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>
#include"datagram.h"
#include<fstream>
#include<vector>
#include<pthread.h>
#include<time.h>


#define SERVER_PORT 8888
#define BUFF_LEN 1024

using namespace std;

typedef struct
{
	int a;
	int b;
} exit_t;

 ofstream file("test1.jpg",ios::binary);

struct sockaddr_in client_addr;
socklen_t len= sizeof(client_addr);
vector<datagram> recdata;//recive buf
static int order=-1;
bool eofflag;
int lastack=0;
/*
    server:
            socket-->bind-->read-->sendto-->close
*/
void udp_msg_process(int fd);
void udp_connect_to_client(int fd, struct sockaddr* to,struct sockaddr_in &client_addr);//fd selfsocket
void getfile(int);
void *savefile(void *);
int acknowledge(int,int);
void savefile(int fd);
    

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
    struct timeval timeout = {1,0};        //to set a rev waiting time
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO, (char *)&timeout,sizeof(struct timeval));//set overtime
    //--------------------------------------------------
    pthread_t tid;
    exit_t *retval;
    int random=0;
    eofflag=false;
    //pthread_create(&tid,NULL,savefile,NULL);
    //cout<<"test"<<endl;
    while(1)
    {       
        
        int count;
        //---------------------------get five packs a time-------------------------
        for(int i=0;i<100;i++)
        {
            
            memset(buf,0,1024);
            count=recv(fd, buf, BUFF_LEN, 0);
            if(count==0)
                break;
            //cout<<"test"<<endl;

            datagram temp;
            memcpy(&temp,buf,sizeof(temp));
            //acknowledge(fd,temp.order);
            if(temp.order>order+100)
                continue;
              //  continue;
            else
            {
                // put the pack into the buf
                if(recdata.size()<=100 )
                    recdata.push_back(temp);
            }
            
            if (temp.end)//end of the file;
                 {
                     eofflag=true;
                     lastack=temp.order;
                     break;
                 }
        }
        //-----------------------------------------
        savefile(fd);

        //the first version---------------
        //acknowledge(fd,order);
        //----------------------------------
        if(eofflag && order==-10)
            break;
        // cout<<"get "<<count<<" bytes"<<endl;    
    }
    //pthread_join(tid, (void **)&retval);
}


void savefile(int fd)
{
    if(!file.is_open())
    {
        cout<<"file not open"<<endl;
    }
    //if(!eofflag)
    {
        if(recdata.empty())
            {
                //cout<<"test"<<endl;
                //sleep(0.01);
            }
        else{
            for(int i=0;i<recdata.size();i++)
            {
                //cout<<"--";
                //--------------------pretend to lose data------------------------
                // if(rand()%2342<50)
                // {
                //     cout<<i<<" "<<"delayed"<<endl;
                //     recdata.clear();
                //     return;
                // }
                //-----------------------------------------------------------------
                if(recdata[i].order==order+1)
                {
                    //cout<<"test"<<endl;
                    if (eofflag && recdata[i].end)
                        {
                            order=-10;
                            //cout<<order<<endl;
                        }
                    else order++;
                    acknowledge(fd,order);
                    //cout<<recdata[i].order<<endl;
                    file.write(recdata[i].d,recdata[i].sendsize);
                    
                    
                }
            }
        }
        recdata.clear();
        
    }
}


int acknowledge(int fd,int ord)
{
    //cout<<order<<endl;
    char buf[4] ;
    memset(buf,0,4);
    int ackback=ord+1;
    if(ord==-10)
        ackback=-10;
    memcpy(buf,&ackback,4);
    //cout<<ord<<endl;
    if(order>-1||ord==-10)
    {
        sendto(fd,buf,4,0,(struct sockaddr*)&client_addr,len);
        cout<<"send ack"<<ackback<<endl;
    }
        
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