#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <iomanip>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

using std::cout;
using std::endl;

void printAddrInfo(addrinfo *pAddrInfo);
uint16_t	icmpChecksum(uint16_t *data, uint32_t len);
void hexdumpBuf(char *buf, uint32_t len);
double getDiffTimeval(const timeval &t1, const timeval &t2);

/*  struct addrinfo {
        int              ai_flags;
        int              ai_family;
        int              ai_socktype;
        int              ai_protocol;
        socklen_t        ai_addrlen;
        struct sockaddr *ai_addr;
        char            *ai_canonname;
        struct addrinfo *ai_next;
    };

    struct sockaddr {
        __uint8_t       sa_len;          //total length
        sa_family_t     sa_family;       //[XSI] address family
        char            sa_data[14];     //[XSI] addr value (actually larger)
    };
*/

int main(int ac, char *av[])
{

    if (ac != 2)
    {
        printf("usage: %s address\n", av[0]);
        return 1;
    }

    addrinfo hints = {0};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    addrinfo *addrInfoLst;

    int ret = getaddrinfo(av[1], NULL, &hints, &addrInfoLst);
    if (ret)
    {
        cout << "ERROR getaddrinfo()" << endl;
        return 1;
    }
    //printAddrInfo(((sockaddr_in*)addrInfoLst->ai_addr)->sin_addr);
    char addrPrint[100];
    inet_ntop(hints.ai_family, &(((sockaddr_in*)addrInfoLst->ai_addr)->sin_addr), addrPrint, 100);


    addrinfo *addrInfoLstFirst = addrInfoLst;

    if (!addrInfoLstFirst)
    {
        cout << "ERROR return getaddrinfo() empty" << endl;
        return 1;
    }

    int sockFd = socket(addrInfoLstFirst->ai_family, addrInfoLstFirst->ai_socktype, addrInfoLstFirst->ai_protocol);
    if (sockFd == -1)
    {
        cout << "ERROR socket(). impossible to create the socket" << endl;
        return 1;
    }

    INADDR_ANY;
    struct sockaddr_in
    {
        __uint8_t sin_len;
        sa_family_t sin_family;
        in_port_t sin_port;
        struct in_addr sin_addr;
        char sin_zero[8];
    };

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;

    //MacOs specificity
    int retBind = bind(sockFd, (sockaddr *) &addr, sizeof(addr));
    if (retBind == -1)
    {
        cout << "retBind error: " << retBind << endl;
    }

    int ttl = 255;
    ret = setsockopt(sockFd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    if (ret == -1)
    {
        perror("perror setsockopt 1");
        cout << "ret: " << ret << endl << endl;
    }

    //struct for sendto()
    icmp icmpPing = {0};
    icmpPing.icmp_type = ICMP_ECHO;
    icmpPing.icmp_code = 0;
    icmpPing.icmp_hun.ih_idseq.icd_id = (uint16_t)getpid();
    icmpPing.icmp_hun.ih_idseq.icd_seq = 0;


    //struct for recvmsg()
    int8_t buf[2048];
    uint8_t bufMsgControl[2048];
    iovec iovec;
    iovec.iov_base = buf;
    iovec.iov_len = 2047;
    msghdr msgRecv = {0};
    msgRecv.msg_name = addrInfoLstFirst->ai_addr;
    msgRecv.msg_namelen = addrInfoLstFirst->ai_addrlen;
    msgRecv.msg_iov = &iovec;
    msgRecv.msg_iovlen = 1;
    msgRecv.msg_control = bufMsgControl;
    msgRecv.msg_controllen = 2048;
    msgRecv.msg_flags = 0;

    cout << "PING " << av[1] << " (" << addrPrint << "): " << sizeof(icmpPing) << " data bytes" << endl;

    bool loop = true;
    while (loop)
    {
        timeval tSend = {0};
        gettimeofday(&tSend, nullptr);
        //TODO verif retour

        ssize_t retSend;
        icmpPing.icmp_cksum = 0;
        icmpPing.icmp_cksum = icmpChecksum((uint16_t *)&icmpPing, sizeof(icmpPing));
        retSend = sendto(sockFd, (void *)&icmpPing, sizeof(icmpPing), 0, addrInfoLstFirst->ai_addr, addrInfoLstFirst->ai_addrlen);
        if (retSend == -1)
        {
            cout << "Error sendto()" << endl;
            return 1;
        }

        ip *ipHeader;
        icmp *icmpHeader;
        bool trash = false;
        do
        {
            ssize_t retRecv = recvmsg(sockFd, &msgRecv, 0);
            if (retRecv == -1)
            {
                cout << "Error recvmsg()" << endl;
                return 1;
            }
            ipHeader = (ip*)buf;
            icmpHeader = (icmp*)(buf + ipHeader->ip_hl * 4);
            if (trash)
                cout << "poubelle" << endl;
            trash = true;
        }
        while (ipHeader->ip_p != IPPROTO_ICMP || icmpHeader->icmp_type != ICMP_ECHOREPLY
                || icmpHeader->icmp_code != 0 || icmpHeader->icmp_hun.ih_idseq.icd_id != (uint16_t)getpid());

        uint8_t ttl = ipHeader->ip_ttl;
        uint8_t length = ipHeader->ip_len + ipHeader->ip_hl * 4;
        timeval tRecv = {0};
        gettimeofday(&tRecv, nullptr);

        cout << (uint16_t)length
        << " bytes from " << addrPrint
        << " icmp_seq=" << icmpPing.icmp_hun.ih_idseq.icd_seq
        << " ttl=" << (uint16_t)ttl
        << " time=" << getDiffTimeval(tSend, tRecv) << " ms" << endl;

        icmpPing.icmp_hun.ih_idseq.icd_seq++;

        sleep(1);
        //loop = false;
    }

    freeaddrinfo(addrInfoLstFirst);
    return 0;
}


double getDiffTimeval(const timeval &t1, const timeval &t2)
{
    double ret = (double)(t2.tv_sec * 1000000 + t2.tv_usec) - (double)(t1.tv_sec * 1000000 + t1.tv_usec);
    ret = (ret / 1000);
    return ret;
}

void hexdumpBuf(char *buf, uint32_t len)
{
    for (int i = 0 ; i < len ; i++)
    {
        cout << std::setw(2) << std::setfill('0') << std::hex << (int)buf[i] << " ";

        if (i % 8 == 7 && i % 16 != 15)
        {
            cout << " ";
        }
        else if (i % 16 == 15)
        {
            cout << endl;
        }
    }
    cout << endl;
}

uint16_t	icmpChecksum(uint16_t *data, uint32_t len)
{
    uint32_t checksum;

    checksum = 0;
    while (len > 1)
    {
        checksum = checksum + *data++;
        len = len - sizeof(uint16_t);
    }
    if (len)
        checksum = checksum + *(uint8_t *)data;
    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum = checksum + (checksum >> 16);
    return (uint16_t)(~checksum);
}

void printAddrInfo(addrinfo *pAddrInfo)
{
    cout << "ai_flags: " << pAddrInfo->ai_flags << endl;
    cout << "ai_family: " << pAddrInfo->ai_family << endl;
    cout << "ai_socktype: " << pAddrInfo->ai_socktype << endl;
    cout << "ai_protocol: " << pAddrInfo->ai_protocol << endl;
    cout << "ai_addrlen: " << pAddrInfo->ai_addrlen << endl;
    if (pAddrInfo->ai_addr)
    {
        cout << "ai_addr->sa_len: " << (int) pAddrInfo->ai_addr->sa_len << endl;
        cout << "ai_addr->sa_family: " << (int) pAddrInfo->ai_addr->sa_family << endl;
        cout << "ai_addr->sa_data[14]: ";
        for (int d = 0; d < 14; d++)
        {
            cout << (int) (uint8_t) pAddrInfo->ai_addr->sa_data[d] << " ";
        }
        cout << endl;
    }
    if (pAddrInfo->ai_canonname)
    {
        pAddrInfo->ai_canonname[14] = 0;
        cout << "ai_canonname: " << pAddrInfo->ai_canonname << endl << endl;
    }
    cout << endl;
}