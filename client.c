#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include<signal.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#define BUFSIZE 1024
int sockfd = 0;


void nw_handler(int signo){          //signal handler
    char a[2];
    if(signo == SIGINT){             //check for ctl + c signal
        printf("pogram received a ctrl+c");
        printf("Are you sure[y/n]");
        scanf("%c",a);
        if(!strcmp("y",a)){
            close(sockfd);           //close socket in case of yes
            exit(0);
        }
        else
            printf("continuing");
    }
}


int main(int argc, char *argv[])    //usage: <./name><ip><port no>
{
    int  n = 0,i;
    struct sockaddr_in serv_addr;

    /* The Sample format to use */
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2

    };

    pa_simple *s = NULL;
    int ret = 1;
    int error;
    int fd;

    if(argc != 3)                   //check for correct usage/arguments
    {
        printf("\n Usage: %s <ip of server> <port number> \n",argv[0]);
        return 1;

    }

    if(signal(SIGINT,nw_handler) == SIG_ERR)    //signal handler callback functio
        printf("cant acces SIDINT");

    //memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)      //create socket
    {
        printf("\n Error : Could not create socket \n");
        return 1;

    }

    //memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;         //set socket IPv4 protocol
    serv_addr.sin_port = htons(atoi(argv[2]));//set port no

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0) //give settings,ip and port no to the opened socket struct
    {
        printf("\n inet_pton error occured\n");
        return 1;

    }

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0 ) //connect to the server
    {
        printf("\n Error : Connect Failed \n");
        return 1;

    }
    while(1){

        uint8_t buf[BUFSIZE];
        ssize_t r;

#if 0
        pa_usec_t latency;

        if ((latency = pa_simple_get_latency(s, &error)) == (pa_usec_t) -1) {
            fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
            goto finish;

        }

        fprintf(stderr, "%0.0f usec    \r", (float)latency);
#endif

        /* Read some data ... */
        if ((r = read(sockfd, buf, sizeof(buf))) <= 0) {
            if (r == 0) /* EOF */
                break;

            fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
            goto finish;

        }

        /* ... and play it */
        if (pa_simple_write(s, buf, (size_t) r, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            goto finish;

        }
    }

    /* Make sure that every single sample was played */
    if (pa_simple_drain(s, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        goto finish;

    }

    ret = 0;

finish:

    if (s)
        pa_simple_free(s);

    return ret;

}
