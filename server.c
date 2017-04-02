#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include "g711.c"

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)
int listenfd = 0,connfd = 0;
#define BUFSIZE 1024
pa_simple *s = NULL;
int pulse_error;
int pulse_rdwt();
/* A simple routine calling UNIX write() in a loop */
static ssize_t loop_write(int fd, const void*data, size_t size) {
    ssize_t ret = 0;
    // printf("start %d\n",(int)size);

    while (size > 0) {
        ssize_t r;

        if ((r = write(fd, data, size)) < 0)
            return r;

        if (r == 0)
            break;

        ret += r;
        data = (const uint8_t*) data + r;
        size -= (size_t) r;
        // close(fd);
        //   printf("end\n");

    }

    return ret;

}

void nw_handler(int signo){         //signal handler
    char a[2];
    if(signo == SIGINT){
        printf("pogram received a ctrl+c");
        printf("Are you sure[y/n]");
        scanf("%c",a);
        if(!strcmp("y",a)){
            close(connfd);      //if continue with ctl + c close the client socket
            close(listenfd);    //close the server socket
            exit(0);
        }
        else
            printf("continuing");
    }
}

static void pulse_handler(int sig, siginfo_t *si, void *uc)
{
    /* Note: calling printf() from a signal handler is not
     *               strictly correct, since printf() is not async-signal-safe;
     *                             see signal(7) */

//    printf("Caught signal %d\n", sig);
    pulse_rdwt();
    //signal(sig, SIG_IGN);

}

int pulse_rdwt(){
    int i;
    uint8_t buf[BUFSIZE];
    uint8_t outmsg[BUFSIZE],inbuffer[BUFSIZE];
    uint16_t inmsg[BUFSIZE],outbuffer[BUFSIZE];
    /* Record some data ... */
    if (pa_simple_read(s, inmsg, sizeof(inmsg), &pulse_error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(pulse_error));
        exit(1);

    }
    /*Conversion of pcm16 to ulaw8 (G711 codec) */
    for(i=0;i<sizeof(inmsg)/sizeof(inmsg[0]);i++)
    {
        outmsg[i]=Snack_Lin2Mulaw(inmsg[i]);

    }
    /* And write it to STDOUT */
    if (loop_write(connfd, outmsg, sizeof(outmsg)) != sizeof(buf)) {
        fprintf(stderr, __FILE__": write() failed: %s\n", strerror(errno));
        exit(1);

    }
}


int main(int argc, char *argv[])    //usage is <./name> <port no>
{
    struct sockaddr_in serv_addr;
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    long long freq_nanosecs;
    sigset_t mask;
    struct sigaction sa;
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    int fd,n,error,ret = 1;

    if(argc != 2)           //check for proper command line arguments
    {
        printf("\n Usage: %s  <port number> \n",argv[0]);
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

    if(signal(SIGINT,nw_handler) == SIG_ERR)    //signal handler call
        printf("cant acces SIDINT");



    listenfd = socket(AF_INET, SOCK_STREAM, 0);     //make the socket
    //memset(&serv_addr, '0', sizeof(serv_addr));
    //memset(sendBuff, '0', sizeof(sendBuff));
    serv_addr.sin_family = AF_INET;                 //socket protocol and settings:IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));      //set port number
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));        //bind the socket with the socket settings struct
    listen(listenfd, 10);       //set the connection limit to 10


    while(1)
    {
        printf("waiting for connection\n");
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);//connect to the client
        printf("connected\n");

        /* Create the recording stream */
        if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
            fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
            goto finish;

        }
        if (timer_settime(timerid, 0, &its, NULL) == -1)
            errExit("timer_settime");
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
            errExit("sigprocmask");

        while(1){

            // write(connfd, sendBuff, strlen(sendBuff)+1);        //write to the client socket

#if 0
            if(n == 0){
                printf("\n******** server terminated *********"); //if server terminate detect and terminate
                close(connfd);
                exit(1);
            }

#endif
        }
        close(connfd);
        sleep(1);
    }

finish:

    if (s)
        pa_simple_free(s);

    return ret;
}
