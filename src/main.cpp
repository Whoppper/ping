#include <iostream>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

int main(int ac, char *av[])
{
    if (ac != 2)
    {
        printf("usage: %s IP address\n", av[0]);
        return 1;
    }
    return 0;
}
