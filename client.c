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
#include <time.h>
#include "g711.c"

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)
#define BUFSIZE 1024
int sockfd = 0;
pa_simple *l = NULL;

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

int pulse_rdply(){
    int error,i;
    uint8_t buf[BUFSIZE];
    uint16_t inmsg[BUFSIZE],outbuffer[BUFSIZE];
    uint8_t outmsg[BUFSIZE],inbuffer[BUFSIZE];
        ssize_t r;
    /* Read some data ... */
    if ((r = read(sockfd, inbuffer, sizeof(inbuffer))) < 0) {
        fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
        exit(0);

    }



    //Conversion of  ulaw8 to pcm16 (G711 codec)
    for(i=0;i<sizeof(inbuffer)/sizeof(inbuffer[0]);i++)
    {
        outbuffer[i]=Snack_Mulaw2Lin(inbuffer[i]);
    }





    //printf("read completed%ld\n",r);
    /* ... and play it */
    if (pa_simple_write(l, outbuffer, sizeof(outbuffer), &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
        exit(0);

    }
}


static void pulse_handler(int sig, siginfo_t *si, void *uc)
{
    /* Note: calling printf() from a signal handler is not
     *               strictly correct, since printf() is not async-signal-safe;
     *                             see signal(7) */

//    printf("Caught signal %d\n", sig);
    pulse_rdply();
    //signal(sig, SIG_IGN);

}

int main(int argc, char *argv[])    //usage: <./name><ip><port no>
{
   // int  n = 0,i;
    struct sockaddr_in serv_addr;
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    long long freq_nanosecs;
    sigset_t mask;
    struct sigaction sa;
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

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = pulse_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIG, &sa, NULL) == -1)
        errExit("sigaction");

    /* Block timer signal temporarily */

    printf("Blocking signal %d\n", SIG);
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
        errExit("sigprocmask");

    /* Create the timer */

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");

    printf("timer ID is 0x%lx\n", (long) timerid);

    /* Start the timer */

    freq_nanosecs = 500;
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

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
    printf("connected\n");

    /* Create a new playback stream */
    if (!(l = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;

    }

        if (timer_settime(timerid, 0, &its, NULL) == -1)
            errExit("timer_settime");
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
            errExit("sigprocmask");

        while(1){

#if 0
        pa_usec_t latency;

        if ((latency = pa_simple_get_latency(s, &error)) == (pa_usec_t) -1) {
            fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
            goto finish;

        }

        fprintf(stderr, "%0.0f usec    \r", (float)latency);
#endif

#if 0
        uint8_t buf[BUFSIZE];
        ssize_t r;
        /* Read some data ... */
        if ((r = read(sockfd, buf, sizeof(buf))) < 0) {
            fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
            exit(0);

        }
        printf("read completed%ld\n",r);
       // printf("read completed\n");
        /* ... and play it */
        if (pa_simple_write(s, buf, (size_t) r, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            printf("hi");
            exit(0);

        }
#endif
        }

    /* Make sure that every single sample was played */
    if (pa_simple_drain(l, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        goto finish;

    }

    ret = 0;

finish:

    if (s)
        pa_simple_free(s);

    return ret;

}
