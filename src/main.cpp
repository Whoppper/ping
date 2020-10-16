#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
//#include "inc/main.h"

using namespace std;

void printAddrInfo(addrinfo *pAddrInfo);

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
    cout << "arg1: " << av[1] << endl;

    int ret = getaddrinfo(av[1], NULL, &hints, &addrInfoLst);
    if (ret)
    {
        cout << "ERROR getaddrinfo()" << endl;
        return 1;
    }
    addrinfo *addrInfoLstFirst = addrInfoLst;

    if (!addrInfoLstFirst)
    {
        cout << "ERROR return getaddrinfo() empty" << endl;
        return 1;
    }

    /*while (addrInfoLst)
    {
        printAddrInfo(addrInfoLst, 0);
        addrInfoLst = addrInfoLst->ai_next;
    }*/


    int sockFd = socket(addrInfoLstFirst->ai_family, addrInfoLstFirst->ai_socktype, addrInfoLstFirst->ai_protocol);
    if (sockFd == -1)
    {
        cout << "ERROR socket(). impossible to create the socket" << endl;
        return 1;
    }

    socklen_t *len = nullptr;
    void *verif = nullptr;
    getsockopt(sockFd, IPPROTO_IP, IP_TTL, verif, len);
    cout << "verif IP_TTL:" << *(int *)verif << endl;

    int ttl = 255;
    setsockopt(sockFd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

    verif = nullptr;
    getsockopt(sockFd, IPPROTO_IP, IP_TTL, verif, len);
    cout << "verif 2 IP_TTL : " << *(int *)verif << endl;

    freeaddrinfo(addrInfoLstFirst);
    return 0;
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
            cout << (int)(uint8_t)pAddrInfo->ai_addr->sa_data[d] << " ";
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