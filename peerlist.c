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
//#include <ncurses.h>

ssize_t RecvN(int sockfd, void *buf, size_t len, int flags){
    fd_set rfds;
    struct timeval tv;
    int retval;
    int read_len = 0;
    char * ptr = (char*)buf;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    while(read_len < len){
        retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
        if (retval == -1){
            perror("select()");
        }
        else if (retval){
            int l;
            l = read(sockfd,ptr,len - read_len);
            printf("l = %u\n",l);
            if(l > 0){
                ptr += l;
                read_len+=l;
            }
            else if(l <= 0){
                return read_len;
            }
        }
        else{
            printf("No data within 2 seconds.\n");
            return 0;
        }
    }

    return read_len;
}

int main(int argc, char * argv[]){
    if(argc != 3){
        puts("Usage ./peer serverip serverPort ");
        exit(0);
    }
    struct sockaddr_in serv_addr;
    int sockfd;
    unsigned argLen = 0;
    char command[100];
    pthread_t thread_accept;
    short serverPort = atoi(argv[2]);
    int fileID = htonl(0x12345678);

    inet_aton(argv[1],&serv_addr.sin_addr);
    serv_addr.sin_port = htons(serverPort);
    serv_addr.sin_family = AF_INET;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
       perror("ERROR connecting");
    }
    command[0] = 0x4;
    command[1] = 1;
    argLen = htonl(4);
    memcpy(command+2,&argLen,4);
    memcpy(command+6,&fileID,4);

    write(sockfd,command,10);
    read(sockfd,command,2);
    printf("msg = %x\n",command[0]);
    printf("len = %d\n",command[1]);

    struct in_addr addr;
    short port = 0;
    int len = command[1];

    for(int i = 0; i < len; i++){
        if(RecvN(sockfd,command,10,0) != 10){
            perror("!!!");
        }
        memcpy(&addr.s_addr,command+4,4);
        memcpy(&port,command+8,2);
        port = ntohs(port);
        char * ip =  inet_ntoa(addr);
        printf("ip = %s\t:%i\t\n",ip,port);

    }
    close(sockfd);
}
