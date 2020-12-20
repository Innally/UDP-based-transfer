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
#include<pthread.h>
#include<assert.h>
#include<math.h>

#define SERVER_PORT 8888
#define BUFF_LEN 1024
#define SERVER_IP "127.0.0.1"
#define WINDOW 1
using namespace std;

int wleft=0;//window wleft
int flag=-1;
const char * filename{"1.jpg"};
ifstream file(filename,ios::binary);

int nNetTimeout=1000;//1秒，
int ack=-1;
int lastack=-1;
int addslide=1;// how many it should slide
int threshold=32;
int winsize=1;
int stop=0;
int allbag=0;
int sameack=0;
vector<datagram> mesarray;
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
void *ifgetack(void* fd);
//when one time didn't get ack, then resent
int resend(int,int);
//send bags, rerturn 1 if the it's the end of the file
bool sendbags(int fd);
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
    clock_t startTime,endTime;
    startTime=clock();
    sendfile(client_fd,filename);
    endTime=clock();
    cout<<"all done"<<endl;
    cout << "The run time is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << endl;
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
    char buf[BUFF_LEN];
    int sendsize=512;
    int order=0;
    //-------------------------open file--------------------------------
    file.seekg( 0, ios::end );
	int filesize = file.tellg();//get how big the file is.
    //cout<<filesize<<endl;
    file.seekg(0,ios::beg);

    double all=(double)filesize/512;
    all=ceil(all);

    cout<<all<<endl;
    //sleep(5);
    allbag=all;
    mesarray.resize(all);
    if(file.is_open())
    {
        cout<<"file successfully oepn."<<endl;
    }
    else {
        cout<<"fail to read file";
        return;
    }
    //---------------------------------------------------------------------

    // read and send the first file block
    // file.read(buf,512);   
    // datagram message(fd,order,buf,sendsize);//datagram get read in message.d
    // memcpy(message.d,buf,sizeof(message.d));
    // mesarray.push_back(message);
    // order++;
    //--------------------thread-------------------------
    //------------------------------------------------

    sendsize=512;
    //load all in
    for(int i=0;i<all;i++)
    {
        memset(buf,0,1024);
        //cout<<file.tellg()<<endl;
        //-------------------------read blocks----------------------------------
        
        //printf("winsize is %d,ack is %d,addslide is %d \n",winsize,ack,addslide);
        //cout<<"read how many blocks "<<readnum<<endl;

        if (flag>=0)
            break;
        if(filesize-file.tellg()>=512)
        {
                    //normally send 512
                    file.read(buf,512);
                    datagram message(fd,order,buf,sendsize);//datagram get read in message.d
                    message.cal_checksum();
                    mesarray[i]=(message);
                    cout<<"message "<<message.order<<endl;
                    //cout<<"test"<<endl;
                    order++;
        }                      // read
        else{
                //when there's no enough data to form a 512 byte data
                sendsize=filesize-file.tellg();
                file.read(buf,sendsize);
                datagram message(fd,order,buf,sendsize);//datagram get read in message.d
                message.end=1;
                message.cal_checksum();
                mesarray[i]=(message);
                cout<<"the last pack:"<<message.order<<endl;
                //flag=i;
                break;
                
        }
        //if threshold <= right- wleft then evety RTT it increase 1;
        //------------------------------------------------------------------------
        //---------send if return 1 then it's the end of the file---------------
        //sendbags(fd);

        //cout<<"the size of massarry "<<mesarray.size()<<endl; 


        //-----------------------mistake or lost check----------------------------
        // ifgetack(fd,ack);
        //------------------------------------------------------------------------
        // addslide=ack-right+5;
        // mesarray.erase(mesarray.begin(),mesarray.begin()+addslide);
        // right=ack+5;

        //mesarray.clear();
    }
    cout<<"out"<<endl;
    
    //sleep(3);
    pthread_t tids;
    int ret = pthread_create(&tids, NULL, ifgetack, &fd);

    while(sendbags(fd));


    if(threshold<=winsize)
        winsize++;

    cout<<"the last"<<file.tellg()<<" and the size of file "<<filesize<<endl;
    cout<<"=====================================";
}

void* ifgetack(void *fdf)
{
    char buf[sizeof(int)] ;
    int fd=*((int *)fdf);
    while(1)
    {
        //memset(buf,0,sizeof(int));
        int ret=recv(fd,buf,sizeof(int),0);
        memcpy(&ack,buf,sizeof(int));
        cout<<"lastack is "<<lastack<<" ack is"<<ack<<endl;
        if(ack==-10)
        {
            flag=1;
            sleep(5);
            return (void*)&flag;
        }
        if(lastack<ack)
        {
            addslide=ack-wleft;
            wleft=lastack=ack;
            
            //====================================
            //cout<<lastack<<" "<<ack<<endl;
            // if (lastack>=ack)
            //     {
            //         resend(fd,ack);
            //         //continue;
            //     }
            //==========================
            //--------------set the wleft and lastack as ack
            
            cout<<"recieved ack:"<<ack<<" addslide "<<addslide<<endl;
            //---------------------------------------------------            
            if(winsize<threshold)
            {
                // everytime it got an ack window get incresed by 1
                winsize++;
                cout<<"winsize++"<<winsize<<endl;
                sleep(3);
            }            
        }
        // else if(lastack==ack)
        // {
        //     resend(fd,lastack);
        //     winsize=1;
        // }
        else if(lastack==ack)
        {
            sameack++;
            if(sameack>=3)
            {
                resend(fd,lastack);
                threshold=winsize/2;
                winsize=threshold+3;
                sameack=0;
            }
        }
        if(ret==0)
        {
            winsize=1;
        }
        // if(ret==0)
        // {
        //     winsize=1;
        //     addslide=0;
        //     continue;
        // }
    }
}

bool sendbags(int fd)
{
    if(flag==1)
    {
        return 0;
    }
    char buf[BUFF_LEN];
    
    for (int i=wleft;i<winsize+wleft;i++)
        {
            memset(buf,0,1024);
            memcpy(buf,&mesarray[i],sizeof(mesarray[i]));   // buf get message
            send(fd,buf,1024,0);
            cout<<"=-=-=this time send "<<mesarray[i].order<<endl;
            
        }
    if(threshold<winsize)
        winsize++;
    return 1;
  
}

int resend(int fd,int s)
{
    int i=0;
    char buf[1024];
    memset(buf,0,1024);
    memcpy(buf,&mesarray[s],sizeof(mesarray[s]));   // buf get message
    cout<<"resend "<<mesarray[s].order<<"ack "<<s <<endl;
    send(fd,buf,1024,0);
    return 0;//fail
}