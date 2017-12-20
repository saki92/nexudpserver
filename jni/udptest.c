#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1100

struct nexio {
    struct ifreq *ifr;
    int sock_rx_ioctl;
    int sock_rx_frame;
    int sock_tx;
};

struct nexio *nexio;

extern struct nexio *nex_init_ioctl(const char *ifname);
extern int nex_ioctl(struct nexio *nexio, int cmd, void *buf, int len, bool set);
unsigned int ioctl_pass(void *custom_cmd_buf, int custom_cmd_buf_len);

char            *ifname = "wlan0";
unsigned int    custom_cmd = 0;
bool            custom_cmd_set = true;
unsigned int    custom_cmd_buf_len;
void            *custom_cmd_buf;

unsigned short low_cmd;
unsigned short high_cmd;


/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(1);
}
unsigned int     ret = 0;
unsigned short *temp_buf;

int main(int argc, char **argv) {
    int     sockfd; /* socket */
    int     portno; /* port to listen on */
    int     clientlen; /* byte size of client's address */
    struct  sockaddr_in serveraddr; /* server's addr */
    struct  sockaddr_in clientaddr; /* client addr */
    struct  hostent *hostp; /* client host info */
    //char    buf[BUFSIZE]; /* message buf */
    //unsigned short    *buf;
    char    *hostaddrp; /* dotted decimal host addr string */
    int     optval; /* flag value for setsockopt */
    int     n; /* message byte size */
    //int ioctl7l = 0x02c7;
    unsigned int    ioctl7l = 711;
    unsigned int     *iip = &ioctl7l;
    unsigned int    *icp = (unsigned int*) iip;
    //char    *bp = (char*)&buf;
    FILE    *Fpointer;
    void            *buf = NULL;

    /*
    * check command line arguments
    */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    buf = malloc(BUFSIZE);
    memset(buf, 0, BUFSIZE);

    /*
    * socket: create the parent socket
    */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
    * us rerun the server immediately after we kill it;
    * otherwise we have to wait about 20 secs.
    * Eliminates "ERROR on binding: Address already in use" error.
    */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
        (const void *)&optval , sizeof(int));

    /*
    * build the server's Internet address
    */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
    * bind: associate the parent socket with a port
    */
    if (bind(sockfd, (struct sockaddr *) &serveraddr,
        sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    /*
    * main loop: wait for a datagram, then echo it
    */
    clientlen = sizeof(clientaddr);
    Fpointer = fopen("sample_write.txt","w");
    fclose(Fpointer);

    while (1) {

        /*
        * recvfrom: receive a UDP datagram from a client
        */
        bzero(buf, BUFSIZE);
        n = recvfrom(sockfd, buf, BUFSIZE, 0,
        (struct sockaddr *) &clientaddr, &clientlen);
        if (n < 0)
            error("ERROR in recvfrom");

        /*
        * gethostbyaddr: determine who sent the datagram
        */
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
            sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");
        printf("server received datagram from %s (%s)\n",
        hostp->h_name, hostaddrp);

        ret = ioctl_pass(buf, n);  
        
        if(ret == 0) {
            printf("ioctl passed successfully!");
        }
        /*if(!(*iip ^ *buf) )
        {
            printf("ioctl match found!");
        }
        */
        
        printf("server received %d bytes: %d\n", n,ret);
        Fpointer = fopen("sample_write.txt","a");
        if(Fpointer == NULL) {
            printf("Unable to open the file");
        }
        if(fwrite(buf,1,BUFSIZE,Fpointer)<0)
        {
            printf("Error writing to file");
        }
        fclose(Fpointer);

      /*
      * sendto: echo the input back to the client
      */
      /*n = sendto(sockfd, buf, strlen(buf), 0,
          (struct sockaddr *) &clientaddr, clientlen);
      if (n < 0)
        error("ERROR in sendto");
      */
    }
}

unsigned int ioctl_pass(void *custom_cmd_buf, int custom_cmd_buf_len) {
    nexio = nex_init_ioctl(ifname);
    temp_buf = (unsigned short *)custom_cmd_buf;
    low_cmd = *temp_buf;
    high_cmd = *(temp_buf+1);

    custom_cmd = low_cmd ^ (high_cmd<<8);
    //printf("%X\n",custom_cmd);
    ret = nex_ioctl(nexio, custom_cmd, custom_cmd_buf, custom_cmd_buf_len, true);
    return ret;
}
