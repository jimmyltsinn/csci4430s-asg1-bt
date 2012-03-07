#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

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

int tracker_test_reply(int sockfd) {
    char msg[2];
    
    printf("-- Reply peer test --\n");
    
    msg[0] = 0x12;
    msg[1] = 0;
    write(sockfd, msg, 2);
    
    getchar();
    printf("-- Reply peer test END --\n");
    return 0;
}

int tracker_reg(int sockfd, int fileID) {
    char msg[12];
    struct sockaddr_in tmp;
    int tmplen = sizeof(tmp);
    char localip[16];
    unsigned short port;

    printf("-- Peer register --\n");

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
    
    getchar();
    printf("-- Peer register END --\n");
    return 0;
}

int tracker_unreg(int sockfd, int fileID) {
    char msg[12];
    struct sockaddr_in tmp;
    int tmplen = sizeof(tmp);
    char localip[16];
    unsigned short port;

    printf("-- Peer unregister --\n");

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
   
    getchar(); 
    printf("-- Peer unregister END --\n");
    return 0;
}

int tracker_list_request(int sockfd) {
    char msg[6];
    int fileID;

//TODO Get the file ID

    fileID = htonl(fileID);
    msg[0] = 0x04;
    msg[1] = 1;
    memcpy(msg + 2, &fileID, 4);
    write(sockfd, msg, 6);

    return 0;
}

int tracker_list_retrive(int sockfd, int argc) {
    unsigned int *ips;
    unsigned short *ports; 
    int i;

    for (i = 0; i < argc; ++i) {
        RecvN(sockfd, ips + i, 4);
        RecvN(sockfd, ports + i, 2);
        printf("[%d] %d.%d.%d.%d : %d", i, (char) *((int*)(ips + i)), (char) *((int*) (ips + i) + 1), (char) *((int*) (ips + i) + 2), (char) *((int*) (ips + i) + 3), ports[i]);
    }
     
    return 0;
}

void tracker_response(int sockfd) {
    char cmd[2];
    RecvN(sockfd, cmd, 2);
    int ret;

    switch(cmd[0]) {
        case 0x11: 
            puts("Registration succeed. ");
            break;
        case 0x21: 
            puts("Registration failed. ");
            break;
        case 0x02: 
            tracker_test_reply(sockfd);
            break;
        case 0x13: 
            puts("Unregistration succeed. ");
            break;
        case 0x14: 
            puts("Retrive download list ... ");
            tracker_list(sockfd, cmd[1]);
            break;
        default: 
            puts("!! I cannot know WTF tracker is saying. ");
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
    
    inet_aton(argv[1], &tracker_ip);
    sockfd = init_connect(tracker_ip, atoi(argv[2]));

    tracker_reg(sockfd, 0x12345678);

    buf = malloc(sizeof(char) * 2);
    RecvN(sockfd, buf, 2);
    if (buf[0] != 0x02)
        goto out;    
    free(buf);

    tracker_test_reply(sockfd);
    buf = malloc(sizeof(char) * 2);
    if (buf[0] != 0x11)
        goto out;
    free(buf);

    tracker_unreg(sockfd, 0x12345678);


    getchar();
out: 
    close(sockfd);

    return 0;
}
