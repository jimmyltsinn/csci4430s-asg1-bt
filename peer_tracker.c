#include "peer.h"

int tracker_reg() {
    char msg[24];
    struct sockaddr_in tracker;
    int tmplen = sizeof(tracker);
    char localip[16];
    int sockfd;
    unsigned int argl, tmp;
    unsigned short port;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Open socket for tracker");
        return -1;
    }
    
    memset(&tracker, 0, sizeof(tracker));
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tracker_ip;
    tracker.sin_port = tracker_port;
   
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("Connect to tracker");
        close(sockfd);
        return -1;
    }

    msg[0] = 0x01;
    msg[1] = 3;

    argl = htonl(4);
    memcpy(msg + 2, &argl, 4);
    memcpy(msg + 2 + 4, &local_ip, 4);
    
    argl = htonl(2);
    port = htons(local_port);
    memcpy(msg + 2 + 4 + 4, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4, &port, 2);
    
    argl = htonl(4);
    tmp = htonl(fileid);
    memcpy(msg + 2 + 4 + 4 + 4 + 2, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4 + 2 + 4, &tmp, 4);

    write(sockfd, msg, 24);
    
    if (read(sockfd, msg, 2) != 2) {
        close(sockfd);
        return -1;
    }
    if (msg[1]) {
        close(sockfd);
        return -1;
    }

    switch (msg[0]) {
        case 0x11: 
            close(sockfd);
            return 0;
        case 0x21: 
            puts("Peer could not be registered to tracker. ");
            return -1;
            break;
        default: ;
    }

    close(sockfd);
    return 0;
}

int tracker_unreg() {
    char msg[24];
    struct sockaddr_in tracker;
    int tmplen = sizeof(tracker);
    char localip[16];
    int sockfd;
    unsigned int argl, tmp;
    unsigned short port;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Open socket for tracker");
        return -1;
    }
    
    memset(&tracker, 0, sizeof(tracker));
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tracker_ip;
    tracker.sin_port = tracker_port;
   
    inet_ntop(AF_INET, &tracker_ip, localip, 16); 
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("Connect to tracker");
        close(sockfd);
        return -1;
    }

    msg[0] = 0x03;
    msg[1] = 3;

    argl = htonl(4);
    memcpy(msg + 2, &argl, 4);
    memcpy(msg + 2 + 4, &tracker.sin_addr, 4);
    
    argl = htonl(2);
    port = htons(local_port);
    memcpy(msg + 2 + 4 + 4, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4, &port, 2);
    
    argl = htonl(4);
    tmp = htonl(fileid);
    memcpy(msg + 2 + 4 + 4 + 4 + 2, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4 + 2 + 4, &tmp, 4);

    write(sockfd, msg, 24);
    
    if (read(sockfd, msg, 2) != 2) {
        close(sockfd);
        return -1;
    }
    if (msg[1]) {
        close(sockfd);
        return -1;
    }

    switch (msg[0]) {
        case 0x13: 
            close(sockfd);
            return 0;
        case 0x23: 
            break;
        default: ;
    }

    close(sockfd);
    return -1;
}

int tracker_list() {
    char msg[10];
    int i, sockfd, n;
    struct sockaddr_in tracker;
    char localip[16];
    unsigned int tmp;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    memset(&tracker, 0, sizeof(tracker));
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tracker_ip;
    tracker.sin_port = tracker_port;
   
    inet_ntop(AF_INET, &tracker_ip, localip, 16); 
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("Connect to tracker");
        close(sockfd);
        return -1;
    }

    msg[0] = 0x04;
    msg[1] = 1;
    tmp = htonl(4);
    memcpy(msg + 2, &tmp, 4);
    tmp = htonl(fileid);
    memcpy(msg + 2 + 4, &tmp, 4);
    write(sockfd, msg, 10);

    read(sockfd, msg, 2);
    if (msg[0] != 0x14) {
        close(sockfd);
        return -1;
    }
    n = msg[1];

    for (i = 0; i < n; ++i) {
        unsigned int tmp;

        if (n >= PEER_NUMBER) {
            break;
        }
        
        read(sockfd, &tmp, 4);
        tmp = ntohl(tmp);
        if (tmp != 6) {
            return -1;
        }
        read(sockfd, &peers_ip[i].s_addr, 4);
        read(sockfd, peers_port + i, 2);
    }
    
    for (; i < PEER_NUMBER; ++i) {
        peers_ip[i].s_addr = 0;
        peers_port[i] = 0;
    }

    close(sockfd);

    return 0;
}

