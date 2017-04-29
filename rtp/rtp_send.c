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
#include <ortp/ortp.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)
#define BUFSIZE 160

pa_simple *s = NULL;
int pulse_error;
int pulse_rdwt();
int runcond=1;
RtpSession *session;
uint32_t user_ts=0;


/* A simple routine calling UNIX write() in a loop */
static ssize_t loop_write(int fd, const void*data, size_t size) {
    ssize_t ret = 0;
    while (size > 0) {
        ssize_t r;
        if ((r = write(fd, data, size)) < 0)
            return r;
        if (r == 0)
            break;
        ret += r;
        data = (const uint8_t*) data + r;
        size -= (size_t) r;
    }
    return ret;
}


/****************************/
void nw_handler(int signo){         //signal handler
    char a[2];
    if(signo == SIGINT){
        runcond = 0;
        rtp_session_destroy(session);
        ortp_exit();
        exit(0);
    }
}
/***************************/

/**************************/
void pulse_handler(int sig, siginfo_t *si, void *uc)
{
    pulse_rdwt();

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

    int a = rtp_session_send_with_ts(session,outmsg,sizeof(outmsg),user_ts);
    user_ts+=sizeof(outmsg);

    //    printf("%d\n",(int)user_ts);
}

int main(int argc, char *argv[])    //usage is <./name> <port no>
{
    struct sockaddr_in serv_addr;
    timer_t timerid;
    char *ssrc;
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

    if(argc < 3)           //check for proper command line arguments
    {
        printf("\n Usage: %s  <ipv4 address> <port number> \n",argv[0]);
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

    freq_nanosecs = 6000;
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if(signal(SIGINT,nw_handler) == SIG_ERR)    //signal handler call
        printf("cant acces SIDINT");

    /*rtp server*/
    ortp_init();
    ortp_scheduler_init();
    ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
    session = rtp_session_new(RTP_SESSION_SENDONLY);
    rtp_session_set_scheduling_mode(session,1);
    rtp_session_set_blocking_mode(session,1);
    rtp_session_set_connected_mode(session,TRUE);
    // rtp_session_set_local_addr(session,argv[1],atoi(argv[2]),atoi(argv[2])+1);
    rtp_session_set_remote_addr(session,argv[1],atoi(argv[2]));
    rtp_session_set_payload_type(session,0);

    ssrc = getenv("SSRC");
    if(ssrc != NULL){
        printf("using SSRC=%i.\n",atoi(ssrc));
        rtp_session_set_ssrc(session,atoi(ssrc));
    }

    /* Create the recording stream */
    if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;

    }
    /*if (timer_settime(timerid, 0, &its, NULL) == -1)
        errExit("timer_settime");
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
        errExit("sigprocmask");
*/
    while(1){

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

        int a = rtp_session_send_with_ts(session,outmsg,sizeof(outmsg),user_ts);
        user_ts+=sizeof(outmsg);

    }

finish:

    if (s)
        pa_simple_free(s);

    return ret;
}
