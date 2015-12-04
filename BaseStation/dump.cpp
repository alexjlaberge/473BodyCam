#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>

#define DUMPER_PORT 8888
#define RECV_BUF_SIZE 1024

using std::cout;
using std::endl;
using std::ios_base;
using std::ofstream;
using std::string;

volatile static bool running = true;

void handle_int(int sig)
{
    if (sig == SIGINT)
    {
        running = false;
        cout << "Received interrupt, stopping..." << endl;
    }
}

int main(int argc, char **argv)
{
    int err;
    int sd;
    ofstream dump;
    string fname{"data.raw"};
    struct pollfd pfd;
    struct sockaddr_in skaddr;

    signal(SIGINT, handle_int);

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("Problem creating socket");
        return 1;
    }

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    skaddr.sin_port = htons(DUMPER_PORT);

    err = bind(sd, (struct sockaddr *) &skaddr, sizeof(skaddr));
    if (err < 0)
    {
        perror("Problem binding");
        return 1;
    }

    if (argc > 1)
    {
        fname = string{argv[1]};
    }

    cout << "Dumping data into " << fname << endl;

    dump.open(fname, ios_base::binary);

    pfd.fd = sd;
    pfd.events = POLLIN;
    while (running)
    {
        char buf[RECV_BUF_SIZE];  

        err = poll(&pfd, 1, 500);
        if (err == 0)
        {
            continue;
        }
        else if (err < 0)
        {
            perror("error polling");
            return 1;
        }

        err = recv(sd, buf, RECV_BUF_SIZE, 0);
        dump.write(buf, err);
    }

    return 0;
}
