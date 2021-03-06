#include "main.h"

bool loop = true;
bool isTimeout = false;

void onSignalReceived(int sig)
{
    if (sig == SIGINT)
    {
        loop = false;
    }
    else if (sig == SIGALRM)
    {
        isTimeout = true;
    }
}

int main(int ac, char *av[])
{

    if (ac != 2)
    {
        printf("usage: %s address\n", av[0]);
        return 1;
    }
    signal(SIGINT, onSignalReceived);
    signal(SIGALRM, onSignalReceived);
    addrinfo hints = {0};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    addrinfo *addrInfoLst;

    int ret = getaddrinfo(av[1], nullptr, &hints, &addrInfoLst);
    if (ret)
    {
        cout << "ping: cannot resolve " << av[1] << ": Unknown host" << endl;
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

    int setSocketTTL = 255;
    ret = setsockopt(sockFd, IPPROTO_IP, IP_TTL, &setSocketTTL, sizeof(setSocketTTL));
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
    uint8_t buf[2048];
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

    int paquetLoss = 0;
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

        //Met l'alarme a 1 seconde pour gerer le timeout
        alarm(1);

        //boucle de receive
        while (!isTimeout)
        {
            ssize_t retRecv = recvmsg(sockFd, &msgRecv, MSG_DONTWAIT);
            if (retRecv == -1)
            {
                //cout << "Error recvmsg() : "<<retRecv << endl;
                //cout << "." << endl
                //return 1;
            }
            if (isEchoReply(buf, retRecv))
            {
                //Annulation de l'alarme
                alarm(0);
                break;
            }
        }

        if (isTimeout)
        {
            //timeout - pas de reponse au ping
            cout << "Request timeout for icmp_seq " << icmpPing.icmp_hun.ih_idseq.icd_seq << endl;
            isTimeout = false;
            paquetLoss++;
        }
        else
        {
            //retour du ping
            ip *ipHeader;
            ipHeader = (ip*)buf;
            uint8_t ttl = ipHeader->ip_ttl;
            uint8_t length = ipHeader->ip_len + ipHeader->ip_hl * 4;
            timeval tRecv = {0};
            gettimeofday(&tRecv, nullptr);

            cout << (uint16_t)length
            << " bytes from " << addrPrint
            << " icmp_seq=" << icmpPing.icmp_hun.ih_idseq.icd_seq
            << " ttl=" << (uint16_t)ttl
            << " time=" << getDiffTimeval(tSend, tRecv) << " ms" << endl;
        }

        icmpPing.icmp_hun.ih_idseq.icd_seq++;

        sleep(1);
    }
    uint16_t paquets = icmpPing.icmp_hun.ih_idseq.icd_seq;
    cout << endl << "--- " << av[1] << " ping statistics ---" << endl;
    cout << icmpPing.icmp_hun.ih_idseq.icd_seq << " packets transmitted, " << paquets - paquetLoss << " received, " <<
            paquetLoss * 100 / paquets << "% packet loss" << endl;
    freeaddrinfo(addrInfoLstFirst);
    return 0;
}

bool isEchoReply(uint8_t *buf, ssize_t retRecv)
{
    ip *ipHeader;
    icmp *icmpHeader;

    if (retRecv >= (long)sizeof(ip))
    {
        ipHeader = (ip*)buf;
        if (ipHeader->ip_p == IPPROTO_ICMP && retRecv >= sizeof(ip) + ipHeader->ip_hl * 4)
        {
            icmpHeader = (icmp*)(buf + ipHeader->ip_hl * 4);
            if (icmpHeader->icmp_type == ICMP_ECHOREPLY && icmpHeader->icmp_code == 0
                && icmpHeader->icmp_hun.ih_idseq.icd_id == (uint16_t)getpid())
            {
                return true;
            }
        }
    }
    return false;
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
