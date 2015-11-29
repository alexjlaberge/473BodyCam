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
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

vector<char> data;

#define MAXBUF 1024*1024

void rec( int sd ) {
    int len,n;
    char bufin[256];
    Mat frame;
    struct sockaddr_in remote;

    len = sizeof(remote);

    while (1) {
	    char bufin [256] = {0};  
	    int size;    
	    n=recvfrom(sd,bufin,MAXBUF,0,(struct sockaddr *)&remote,(unsigned int*)&len);
	    printf("%s", bufin);
	    if(!strcmp(bufin, "yo new image\n"))
	    {
		    printf("startinggggg\n");
		
            //FILL THE BUFFER
            while(strncmp(bufin, "kthxbai\n", 8) != 0)
		    {
                char bufin [256] = {0};			
                n=recvfrom(sd,bufin,MAXBUF,0,(struct sockaddr *)&remote,(unsigned int*)&len);
                for(int i = 0; i < n; i++)
                {
                    data.push_back(*(bufin+i));
                    printf("%c\n", *(bufin+i));
                }
                //printf("%s", bufin);
		    }
            
            //Decode vector into frame
            //frame = imdecode(Mat(data), 1);
            
            //Show the frame
            //imshow("Police Video", frame);
            
            //Clear the data vector
            data.clear();
	}      
    }
    
}

int main() {
    int ld;
    struct sockaddr_in skaddr;
    int length;

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
