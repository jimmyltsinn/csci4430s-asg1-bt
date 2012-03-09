#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

unsigned int fileID = 0x12345678;
//struct in_addr tip;
//unsigned short tport = 0;
//unsigned int listen_port = 0;

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
        } else if (retval) {
            int l;
            l = read(sockfd, ptr, len - read_len);
            if (l > 0) {
                ptr += l;
                read_len += l;
            } else if (l <= 0) {
                return read_len;
            }
        } else {
            printf("RecvN() [%d] No data within 2 secs. \n", sockfd);
            return 0;
        }
    }
    printf("Reaching the end of RecvN() [%d] ... Something goes wrong ??\n", sockfd);
    return read_len;
}

/*
int init_connect(struct in_addr ip, const unsigned short port) {
    int sockfd = -1;
    struct sockaddr_in tgt;
    char tgtip[16];


    tgt.sin_addr = ip;
    tgt.sin_port = htons(port);
    tgt.sin_family = AF_INET;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    inet_ntop(AF_INET, &ip, tgtip, 16);

    fprintf(stderr, "[i-c]\tTarget: %s : %d\n", tgtip, ntohs(port));

    if (connect(sockfd, (struct sockaddr*) &tgt, sizeof(tgt)) < 0) {
        perror("init_connect() -> connect()");
        close(sockfd);
        return -1;
    }

    return sockfd;
}
*/

int tracker_reg(struct in_addr tip, in_port_t tport, in_port_t listen_port) {
    char msg[24];
    struct sockaddr_in tracker;
    int tmplen = sizeof(tracker);
    char localip[16];
    int sockfd;
    unsigned int argl, tmp;
    unsigned short port;

    fprintf(stderr, "-- Peer register --\n");
    fprintf(stderr, "\tConnection\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[REG] socket()");
        return -1;
    }
    
    memset(&tracker, sizeof(tracker), 0);
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tip;
    tracker.sin_port = htons(tport);
   
    inet_ntop(AF_INET, &tip, localip, 16); 
    printf("IP = %s : %d\n", localip, tport);
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("[REG] connect()");
        close(sockfd);
        return -1;
    }

    fprintf(stderr, "\tCompose the message\n");
    msg[0] = 0x01;
    msg[1] = 3;

    argl = htonl(4);
    memcpy(msg + 2, &argl, 4);
    memcpy(msg + 2 + 4, &tracker.sin_addr, 4);
    
    argl = htonl(2);
    port = htons(listen_port);
    memcpy(msg + 2 + 4 + 4, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4, &port, 2);
    
    argl = htonl(4);
    tmp = htonl(fileID);
    memcpy(msg + 2 + 4 + 4 + 4 + 2, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4 + 2 + 4, &tmp, 4);

    fprintf(stderr, "\tSent the message\n");
    write(sockfd, msg, 24);
    
    fprintf(stderr, "\tReceive the return\n");
    if (read(sockfd, msg, 2) != 2) {
        puts("No response ...");
        close(sockfd);
        return -1;
    }
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

    fprintf(stderr, "\tClose the socket\n");
    close(sockfd);
    return -1;
}

int tracker_unreg(struct in_addr tip, in_port_t tport, in_port_t listen_port) {
    char msg[24];
    struct sockaddr_in tracker;
    int tmplen = sizeof(tracker);
    char localip[16];
    int sockfd;
    unsigned int argl, tmp;
    unsigned short port;

    fprintf(stderr, "-- Peer unregister --\n");
    fprintf(stderr, "\tConnection\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[UNREG] socket()");
        return -1;
    }
    
    memset(&tracker, sizeof(tracker), 0);
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tip;
    tracker.sin_port = htons(tport);
   
    inet_ntop(AF_INET, &tip, localip, 16); 
    printf("IP = %s : %d\n", localip, tport);
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("[UNREG] connect()");
        close(sockfd);
        return -1;
    }

    fprintf(stderr, "\tCompose the message\n");
    msg[0] = 0x03;
    msg[1] = 3;

    argl = htonl(4);
    memcpy(msg + 2, &argl, 4);
    memcpy(msg + 2 + 4, &tracker.sin_addr, 4);
    
    argl = htonl(2);
    port = htons(listen_port);
    memcpy(msg + 2 + 4 + 4, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4, &port, 2);
    
    argl = htonl(4);
    tmp = htonl(fileID);
    memcpy(msg + 2 + 4 + 4 + 4 + 2, &argl, 4);
    memcpy(msg + 2 + 4 + 4 + 4 + 2 + 4, &tmp, 4);

    fprintf(stderr, "\tSent the message\n");
    write(sockfd, msg, 24);
    
    fprintf(stderr, "\tReceive the return\n");
    if (read(sockfd, msg, 2) != 2) {
        puts("No response ...");
        close(sockfd);
        return -1;
    }
    if (msg[1]) {
        puts("Wrong type response ... Check the tracker. b");
        close(sockfd);
        return -1;
    }

    switch (msg[0]) {
        case 0x13: 
            puts("Peer has unregistered from tracker. ");
            close(sockfd);
            return 0;
        case 0x23: 
            puts("Peer could not be unregistered from tracker. ");
            break;
        default: 
            puts("Wrong type response ... Check the tracker. c");
    }

    fprintf(stderr, "\tClose the socket\n");
    close(sockfd);
    return -1;
}

int tracker_list(struct in_addr tip, in_port_t tport) {
    char msg[10];
    int i, sockfd, n;
    struct sockaddr_in tracker;
    char localip[16];
    unsigned int tmp;
    unsigned int *ips;
    unsigned short *ports;

    puts("-- Tracker List Request --");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[LIST] socket()");
        return -1;
    }
    
    memset(&tracker, sizeof(tracker), 0);
    tracker.sin_family = AF_INET;
    tracker.sin_addr = tip;
    tracker.sin_port = htons(tport);
   
    inet_ntop(AF_INET, &tip, localip, 16); 
    printf("IP = %s : %d\n", localip, tport);
    if (connect(sockfd, (struct sockaddr*) &tracker, sizeof(tracker)) < 0) {
        perror("[LIST] connect()");
        close(sockfd);
        return -1;
    }

    msg[0] = 0x04;
    msg[1] = 1;
    tmp = htonl(4);
    memcpy(msg + 2, &tmp, 4);
    tmp = htonl(fileID);
    memcpy(msg + 2 + 4, &tmp, 4);
    write(sockfd, msg, 10);

    read(sockfd, msg, 2);
//    RecvN(sockfd, msg, 2);
    if (msg[0] != 0x14) {
        puts("Wrong type response ... Check the tracker. ");
        close(sockfd);
        return -1;
    }
    n = msg[1];
    printf("Number of registered peer = %d\n", n);

    ips = malloc(sizeof(unsigned int) * n);
    ports = malloc(sizeof(unsigned short) * n);

    for (i = 0; i < n; ++i) {
        unsigned int tmp;
        char ip[16];

        read(sockfd, &tmp, 4);
        tmp = ntohl(tmp);
        if (tmp != 6) {
            printf("tmp = %x\n", tmp);
            puts("Wrong format in message length ... ");
            return -1;
        }
        read(sockfd, ips + i, 4);
        read(sockfd, ports + i, 2);
       

        ports[i] = ntohs(ports[i]);
        inet_ntop(AF_INET, ips + i, ip, 16);
        printf("[%d] %s : %d\n", i, ip, ports[i]); 
    }

    close(sockfd);

    return 0;
}

void test_reply(int sockfd) {
    char msg[2];
    
    fprintf(stderr, "Reply test message to tracker\n"); 
    msg[0] = 0x12;
    msg[1] = 0;

    write(sockfd, msg, 2);

    return;
}

void thread_response(int sockfd) {
    fprintf(stderr, "-- Response to a connection --\n");
    char msg[2];
    
    fprintf(stderr, "\tWaiting for message return\n");
    if (!read(sockfd, msg, 2)) {
        fprintf(stderr, "Could not receive enough length of command\n");   
        pthread_exit(NULL);
    }
    
    switch (msg[0]) {
        case 0x02: 
            if (!msg[1]) {
                test_reply(sockfd);
                close(sockfd);
                break;
            }
        default: 
            puts("Receiving a unknown command, ignoring it");
            close(sockfd);
    }
    pthread_exit(0);
    return;
}

void thread_accept(in_port_t lport) {
    int sockfd, income_sockfd;
    struct sockaddr_in local_addr, income_addr;
    socklen_t income_addr_len;
    int n = 0;
    char tmp[10];
    pthread_t threads[5];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[ACCEPT] socket()");
        pthread_exit(0);
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(lport);
    if(bind(sockfd, (struct sockaddr *) &local_addr, sizeof(local_addr)) < 0) {
        perror("[ACCEPT] bind()");
        pthread_exit(0);
    }

    listen(sockfd, 5);

    printf("Listening port opened: %d (sockfd = %d)\n", lport, sockfd);
    while (1) {
        income_addr_len = sizeof(income_addr);
        
        fprintf(stderr, "Accept ...\n");
        income_sockfd = accept(sockfd, (struct sockaddr*) &income_addr, &income_addr_len);
        fprintf(stderr, "Accepted. sockfd = %d\n", income_sockfd);
        if (income_sockfd < 0) {
            perror("[ACCEPT] accept()");
            pthread_exit(0);
        }

        pthread_create(threads + n, NULL, (void * (*) (void *)) thread_response, income_sockfd);
        ++n;
    }
    pthread_exit(0);
    return;
}

int main(int argc, char **argv) {
    int sockfd;
    char *buf;
    pthread_t accept;
    struct in_addr tip;
    in_port_t tport, listen_port;

    if (argc != 4) {
        printf("Usage: %s serverIP serverPort myPort\n", argv[0]);
        exit(0);
    }
    
    fprintf(stderr, "Tracker: %s : %d\n", argv[1], atoi(argv[2]));
 
    inet_aton(argv[1], &tip);
    tport = atoi(argv[2]);
    listen_port = atoi(argv[3]);

    pthread_create(&accept, NULL, (void* (*) (void*)) thread_accept, (void*) listen_port);

    tracker_reg(tip, tport, listen_port);

    getchar();
    
    tracker_list(tip, tport);

    getchar();

    tracker_unreg(tip, tport, listen_port);
    while (1);
out: 
    close(sockfd);

    return 0;
}

