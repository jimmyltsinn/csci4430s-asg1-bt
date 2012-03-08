#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ncurses.h>
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

ssize_t RecvN(int sockfd, void *buf, size_t len, int flags) {
    fd_set rfds;
    struct timeval tv;
    int retval;
    int read_len = 0;
    char *ptr = (char*)buf;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    while (read_len < len) {
        retval = select(1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select()");
        } else if (retval) {
            int l;
            l = read(sockfd,ptr,len - read_len);
            if (l > 0) {
                ptr += l;
                read_len+=l;
            } else if (l <= 0) {
                return read_len;
            }
        } else {
            printf("No data within 2 seconds.\n");
            return 0;
        }
    }

    return 0;
}

void accept_thread (int portno) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("accept_thread() -> socket()");
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0) {
        error("accept_thread() -> bind()");
        exit(0);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
            (struct sockaddr *) &cli_addr, 
            &clilen);
    if (newsockfd < 0){
        error("accept_thread() -> accept()");
    }
    {
        char msg[2];
        read(newsockfd,&msg,2);
        if(msg[0] != 0x2){
            printf("MSG = %x\n",msg[0]);
        }
        msg[0] = 0x12;
        write(newsockfd,&msg,2);
        close(newsockfd);
    }
    printf("YES!!");
    pthread_exit(0);
    ///...
}

int main(int argc, char * argv[]){
    struct sockaddr_in serv_addr;
    int sockfd;
    char command[100], *ptr;
    unsigned len;
    pthread_t thread_accept;
    short myPort = atoi(argv[3]);
    short serverPort = atoi(argv[2]);
    int fileID = htonl(0x12345678);
    
    if (argc != 4) {
        printf("Usage ./%s serverip serverPort myPort\n", argv[0]);
        exit(0);
    }
    
    pthread_create(&thread_accept, NULL, accept_thread, myPort);

    inet_aton(argv[1], &serv_addr.sin_addr);
    serv_addr.sin_port = htons(serverPort);
    serv_addr.sin_family = AF_INET;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
       perror("main() -> connect()");
    }
    
    command[0] = 0x1;
    command[1] = 3;
    ptr = command+2;

    myPort = htons(myPort);
    len = htonl(4);
    memcpy(ptr,&len,4); ptr+=4;
    memcpy(ptr,&serv_addr.sin_addr,4);ptr+=4;

    len = htonl(2);
    memcpy(ptr,&len,4); ptr+=4;
    memcpy(ptr,&myPort,2); ptr+=2;

    len = htonl(4);
    memcpy(ptr,&len,4); ptr+=4;
    memcpy(ptr,&fileID,4); ptr+=4;

    printf("sending len = %u\n",ptr - command);
    write(sockfd,command,ptr - command);
    read(sockfd,command,2);
    printf("msg = %x\n",command[0]);
    close(sockfd);
    printf("End of main\n");
    while(1);
    return 0;
}
