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
#include <ortp/ortp.h>
#include <bctoolbox/vfs.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)
#define BUFSIZE 160
pa_simple *l = NULL;
int cond =0;
RtpSession *session;
uint32_t ts=0;

void ssrc_cb(RtpSession *session){
    printf("hey, the ssrc has changed !\n");
}
void nw_handler(int signo){          //signal handler
    char a[2];
    cond = 0;
    if(signo == SIGINT){             //check for ctl + c signal
            rtp_session_destroy(session);
            ortp_exit();
            exit(-1);
    }
}

int pulse_rdply(){
    int error,err=1,i,h=1;
    uint8_t buf[BUFSIZE];
    uint16_t inmsg[BUFSIZE],outbuffer[BUFSIZE];
    uint8_t outmsg[BUFSIZE],inbuffer[BUFSIZE];
        ssize_t r;
    /* Read some data ... */

			err=rtp_session_recv_with_ts(session,inbuffer,sizeof(inbuffer),ts,&h);
			/* this is to avoid to write to disk some silence before the first RTP packet is returned*/

		ts+=sizeof(inbuffer);
    //rtp_session_recm_with_ts(session,inbuffer,sizeof(inbuffer),ts,&h);
    err = rtp_session_recv_with_ts(session,inbuffer,sizeof(inbuffer),ts,&h);
    //printf("%d",error);
    if(err>0){
    ts+=sizeof(inbuffer);
    printf("%d %d\n",ts,err);
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
}


static void pulse_handler(int sig, siginfo_t *si, void *uc)
{
    pulse_rdply();

}

int main(int argc, char *argv[])    //usage: <./name><ip><port no>
{
   // int  n = 0,i;
    int local_port;
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

    if(argc < 2)                   //check for correct usage/arguments
    {
        printf("\n Usage: %s  <port number> \n",argv[0]);
        return 1;

    }
    local_port = atoi(argv[2]);
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

    freq_nanosecs = 6000;
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if(signal(SIGINT,nw_handler) == SIG_ERR)    //signal handler callback functio
        printf("cant acces SIDINT");

    ortp_init();
    ortp_scheduler_init();
    ortp_set_log_level_mask(ORTP_DEBUG|ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
    session = rtp_session_new(RTP_SESSION_RECVONLY);
    rtp_session_set_scheduling_mode(session,1);
    rtp_session_set_blocking_mode(session,1);
    rtp_session_set_local_addr(session,argv[1],local_port,local_port+1);
    rtp_session_set_connected_mode(session,TRUE);
    rtp_session_set_symmetric_rtp(session,TRUE);
    rtp_session_set_payload_type(session,0);
    //rtp_session_signal_connect(session,"ssrc changed",(RtpCallback)ssrc_cb,0);
    //rtp_session_signal_connect(session,"ssrc changed",(RtpCallback)rtp_session_reset,0);
    /* Create a new playback stream */
    if (!(l = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }

       /* if (timer_settime(timerid, 0, &its, NULL) == -1)
            errExit("timer_settime");
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
            errExit("sigprocmask");
*/
        while(1){

            int error,err=1,i,h=1;
            uint8_t buf[BUFSIZE];
            uint16_t inmsg[BUFSIZE],outbuffer[BUFSIZE];
            uint8_t outmsg[BUFSIZE],inbuffer[BUFSIZE];
            ssize_t r;
            /* Read some data ... */

            err=rtp_session_recv_with_ts(session,inbuffer,sizeof(inbuffer),ts,&h);
            /* this is to avoid to write to disk some silence before the first RTP packet is returned*/

            ts+=sizeof(inbuffer);
            //rtp_session_recm_with_ts(session,inbuffer,sizeof(inbuffer),ts,&h);
            err = rtp_session_recv_with_ts(session,inbuffer,sizeof(inbuffer),ts,&h);
            //printf("%d",error);
            if(err>0){
                ts+=sizeof(inbuffer);
                printf("%d %d\n",ts,err);
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
        }


finish:

    if (s)
        pa_simple_free(s);

    return ret;

}
