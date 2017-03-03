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

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

int listenfd = 0,connfd = 0;
#define BUFSIZE 1024

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

int main(int argc, char *argv[])    //usage is <./name> <port no>
{
    struct sockaddr_in serv_addr;

    /* The sample type to use */
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2

    };
    pa_simple *s = NULL;
    int fd,n,error,ret = 1;

    if(argc != 2)           //check for proper command line arguments
    {
        printf("\n Usage: %s  <port number> \n",argv[0]);
        return 1;

    }

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
        while(1){

            uint8_t buf[BUFSIZE];

            /* Record some data ... */
            if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
                fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
                goto finish;

            }

            /* And write it to STDOUT */
            if (loop_write(connfd, buf, sizeof(buf)) != sizeof(buf)) {
                fprintf(stderr, __FILE__": write() failed: %s\n", strerror(errno));
                goto finish;

            }
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
