#include<cstring>


class datagram
{
public:
    int ack;
    int srcsock;
    int order;  //the order
    char d[512];
    int sendsize;
    unsigned short checksum;
    
public:
    datagram(int& ,int&,char*,int&);
    datagram(){};
    ~datagram();

public:
     unsigned short cal_checksum();
};

datagram::datagram(int& src,int& order,char* d,int& sendsize)
{
    memset(this->d,0,512);
    this->srcsock=src;
    this->order=order;
    this->sendsize=sendsize;
    memcpy(this->d,d,512);
    checksum=0;
}

datagram::~datagram()
{
}


unsigned short datagram::
cal_checksum()
{
    unsigned short* buf=(unsigned short*)&d;
    int count=sizeof(buf)/sizeof(short);
    unsigned long sum = 0;
    while (count--)
    {
        sum += *buf++;
        if (sum & 0XFFFF0000)
        {
            sum &= 0XFFFF;
            sum++;
        }
    }
    checksum=~(sum & 0XFFFF);
    return ~(sum & 0XFFFF);
}


