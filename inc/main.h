//
// Created by Sylvain Caussinus on 16/10/2020.
//

#ifndef PING_MAIN_H
#define PING_MAIN_H

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <iomanip>
#include <time.h>
#include <signal.h>
#include <sys/time.h>

uint16_t	icmpChecksum(uint16_t *data, uint32_t len);
void        hexdumpBuf(char *buf, uint32_t len);
double      getDiffTimeval(const timeval &t1, const timeval &t2);
bool        isEchoReply(uint8_t *buf, ssize_t retRecv);

using std::cout;
using std::endl;


#endif //PING_MAIN_H
