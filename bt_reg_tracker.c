#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

union {
    int whole;
    char sub[4];
} ip;

unsigned int fileID = 0;
struct in_addr tip;
unsigned short tport = 0;

ssize_t RecvN(int sockfd, void *buf, size_t len) {
    fd_set rfds;
    struct timeval tv;
    int retval;
    int read_len = 0;
    char *ptr = (char*) buf;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    while (read_len < len) {
        retval = select(1, &rfds, NULL, NULL, &tv);
        if (retval < 0) {
            perror("select()");
            return read_len;
        }
        if (retval) {
            int l;
            l = read(sockfd, ptr, len - read_len);
            if (l > 0) {
                ptr += l;
                read_len += l;
            } else {
                return read_len;
            }
        } else {
            printf("RecvN() [%d] No data within 2 secs. \n", sockfd);
            return read_len;
        }
    }
    printf("Reaching the end of RecvN() [%d] ... Something goes wrong ??\n", sockfd);
    return read_len;
}

int init_connect(struct in_addr ip, const unsigned short port) {
    int sockfd = -1;
    struct sockaddr_in tgt;

    tgt.sin_addr = ip;
    tgt.sin_port = htons(port);
    tgt.sin_family = AF_INET;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sockfd, (struct sockaddr*) &tgt, sizeof(tgt)) < 0) {
        perror("init_connect() -> connect()");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int tracker_reg() {
    char msg[12];
    struct sockaddr_in tmp;
    int tmplen = sizeof(tmp);
    char localip[16];
    unsigned short port;
    int sockfd; 

    printf("-- Peer register --\n");

    sockfd = init_connect(tip, tport);

    if (sockfd < 0)
        return -1;

    if (getsockname(sockfd, (struct sockaddr*)&tmp, &tmplen)) {
        perror("getsockname()");
        return -1;
    }
    
    if(!inet_ntop(AF_INET, &tmp.sin_addr, localip, 16)) {
        perror("inet_ntop()");
        return -1;
    }
     
    msg[0] = 0x01;
    msg[1] = 3;

    memcpy(msg + 2, &tmp.sin_addr, 4);
    
    port = htons(tmp.sin_port);
    memcpy(msg + 2 + 4, &port, 2);
    
    fileID = htons(fileID);
    memcpy(msg + 2 + 4 + 2, &fileID, 4);
    
    write(sockfd, msg, 12);
   
    RecvN(sockfd, msg, 2);
    if (msg[0] != 0x02 || msg[1]) {
        puts("Wrong type response ... Check the tracker. a");
        close(sockfd);
        return -1;
    }

    msg[0] = 0x12;
    msg[1] = 0;
    write(sockfd, msg, 2);

    RecvN(sockfd, msg, 2);
    if (msg[1]) {
        puts("Wrong type response ... Check the tracker. b");
        close(sockfd);
        return -1;
    }

    switch (msg[0]) {
        case 0x11: 
            puts("Peer has registered to tracker successfully. ");
            close(sockfd);
            return 0;
        case 0x21: 
            puts("Peer could not be registered to tracker. ");
            break;
        default: 
            puts("Wrong type response ... Check the tracker. c");
    }

    close(sockfd);
    return -1;
}

int tracker_unreg() {
    char msg[12];
    struct sockaddr_in tmp;
    int tmplen = sizeof(tmp);
    char localip[16];
    unsigned short port;
    int sockfd;

    printf("-- Peer unregister --\n");

    sockfd = init_connect(tip, tport);

    if (sockfd < 0)
        return -1;
    
    if (getsockname(sockfd, (struct sockaddr*)&tmp, &tmplen)) {
        perror("getsockname()");
        return -1;
    }
    
    if(!inet_ntop(AF_INET, &tmp.sin_addr, localip, 16)) {
        perror("inet_ntop()");
        return -1;
    }
    
    msg[0] = 0x01;
    msg[1] = 3;

    memcpy(msg + 2, &tmp.sin_addr, 4);
    
    port = htons(tmp.sin_port);
    memcpy(msg + 2 + 4, &port, 2);
    
    fileID = htons(fileID);
    memcpy(msg + 2 + 4 + 2, &fileID, 4);
    
    write(sockfd, msg, 12);
   
    RecvN(sockfd, msg, 2);
    if (msg[0] != 0x13 || msg[1]) {
        puts("Wrong type response ... Check the tracker. ");
        return -1;
    }
    
    puts("Peer has successfully unregistered from tracker. ");
    return 0;
}

int tracker_list() {
    char msg[6];
    int i, sockfd, n;

    unsigned int *ips;
    unsigned short *ports;


    puts("-- Tracker List Request --");
    sockfd = init_connect(tip, tport);

    if (sockfd < 0)
        return -1;

    fileID = htonl(fileID);
    msg[0] = 0x04;
    msg[1] = 1;
    memcpy(msg + 2, &fileID, 4);
    write(sockfd, msg, 6);

    RecvN(sockfd, msg, 2);
    if (msg[0] != 0x14) {
        puts("Wrong type response ... Check the tracker. ");
        close(sockfd);
        return -1;
    }
    n = msg[1];

    ips = malloc(sizeof(unsigned int) * n);
    ports = malloc(sizeof(unsigned short) * n);

    for (i = 0; i < n; ++i) {
        char tmp[4];

        read(sockfd, ips + i, 4);
        read(sockfd, ports + i, 2);
        
        *tmp = ntohl(ips[i]);

        printf("[%d] %d.%d.%d.%d : %d\n", i, tmp[0], tmp[1], tmp[2], tmp[3], ntohs(ports[i]));
    }

    close(sockfd);

    return 0;
}

int main(int argc, char **argv) {
    int sockfd;
    struct in_addr tracker_ip;
    char *buf;

    if (argc != 4) {
        printf("Usage: %s serverIP serverPort myPort\n", argv[0]);
        exit(0);
    }
    
    inet_aton(argv[1], &tip);
    tport = htons(atoi(argv[2]));

    tracker_reg();

    getchar();
out: 
    close(sockfd);

    return 0;
}
