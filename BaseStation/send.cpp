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

#define BUF_SIZE 128
#define INADDR_LOCALHOST 0x7F000001

using std::cout;
using std::endl;
using std::ifstream;
using std::ios_base;
using std::string;

int main(int argc, char **argv)
{
    ifstream data;
    int err;
    int sd;
    string fname{"data.raw"};
    struct sockaddr_in destination;
    uint8_t buf[BUF_SIZE];

    if (argc > 1)
    {
        fname = string{argv[1]};
    }

    data.open(fname, ios_base::binary);
    if (!data.good())
    {
        cout << "could not open file" << endl;
        return 1;
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        perror("could not create socket");
        return 1;
    }

    destination.sin_family = AF_INET;
    destination.sin_addr.s_addr = htonl(INADDR_LOCALHOST);
    destination.sin_port = htons(8888);
    err = connect(sd, (struct sockaddr *) &destination, sizeof(destination));
    if (err != 0)
    {
        perror("could not set socket default address");
        return 1;
    }

    while (!data.eof())
    {
        data.read(reinterpret_cast<char *>(buf), BUF_SIZE);
        err = send(sd, buf, data.gcount(), 0);
        if (err < 0)
        {
            perror("error sending");
            return 2;
        }
        cout << "sent " << err << endl;

        usleep(1000);
    }

    return 0;
}
