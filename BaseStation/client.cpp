/* UDP echo server program -- echo-server-udp.c */

#include <stdio.h>      /* standard C i/o facilities */
#include <stdlib.h>     /* needed for atoi() */
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions */
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */
#include <string.h>
#include <signal.h>
#include <fstream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

vector<char> data (38400);
volatile static bool running = true;

#define MAXBUF 1024*1024

void handle_int(int sig)
{
    if (sig == SIGINT)
    {
        running = false;
    }
}

void rec( int sd ) {
    int len,n;
    Mat frame;
    struct sockaddr_in remote;
    ofstream dump{"data.raw"};

    len = sizeof(remote);

    while (1) {
	    char bufin[256] = {0};  
	    uint32_t len;
	    uint32_t offset;
	    
	    n = recvfrom(sd,bufin,MAXBUF,0,(struct sockaddr *)&remote,(unsigned int*)&len);
	    dump.write(bufin, n);
	    //printf("%s\n", bufin);
	    
	    if (0 == strcmp(bufin, "kthxbye"))
	    {
	        std::cout << "ending with " << data.size() << std::endl;
	        while (data.size() < 38400)
	        {
	            data.push_back(0);
	        }
	        while (data.size() > 38400)
	        {
	            data.pop_back();
	        }
	        frame = Mat(Size(160, 120), CV_8UC2, data.data());
	        //cvtColor(frame, frame, CV_YCbCr2RGB);
	        cvtColor(frame, frame, COLOR_YUV2BGR_YUY2);
	        //cvtColor(frame, frame, COLOR_YUV2BGR_YUYV);
	        //cvtColor(frame, frame, COLOR_YUV2BGR_UYVY);
	        //cvtColor(frame, frame, COLOR_YUV2BGR_YVYU);
	        //cvtColor(frame, frame, COLOR_YUV2BGR_VYUY);
	        imshow("Police Video", frame);
	        waitKey(20);
	        //data.clear();
	        
	        if (running == false)
	        {
	            break;
	        }
	        
	        continue;
	    }

        if (0 == strcmp(bufin, "yo new image"))
        {
            std::cout << "starting" << std::endl;
            //data.clear();
            continue;
        }
        
        //for (size_t i = 0; i < n; i++)
        //{
            //Get offset
            offset = *((uint32_t *) bufin);
            len = *((uint32_t *) (bufin + 4));
            
            //cout << offset << endl;
            //cout << len << endl;
            
            //data.push_back(bufin[i]);
            for(int i = 8; i < n; i += 2)
            {
                data[offset + i - 8] = bufin[i];
            }
        //}
	}
}

int main() {
    int ld;
    struct sockaddr_in skaddr;
    int length;
    
    signal(SIGINT, handle_int);

    if ((ld = socket( AF_INET, SOCK_DGRAM, 0 )) < 0) {
        printf("Problem creating socket\n");
        exit(1);
    }

    skaddr.sin_family = AF_INET;
    skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    skaddr.sin_port = htons(8888);

    if (bind(ld, (struct sockaddr *) &skaddr, sizeof(skaddr))<0) {
        printf("Problem binding\n");
        exit(0);
    }
  
    length = sizeof( skaddr );
  
    if (getsockname(ld, (struct sockaddr *) &skaddr, (unsigned int*)&length)<0) {
        printf("Error getsockname\n");
        exit(1);
    }
  
    printf("The server UDP port number is %d\n",ntohs(skaddr.sin_port));

    rec(ld);
    return(0);

}
