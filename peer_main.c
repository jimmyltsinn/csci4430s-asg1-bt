#include "peer.h"

void tracking_thread() {
    while (1) {
        int i;
        
        tracker_list();
        for (i = 0; i < PEER_NUMBER; ++i) {
            memset(peers_bitmap[i], 0, (nchunk + 8) >> 3);
            if (!peers_ip[i].s_addr)
                continue;
            peer_getbitmap(i);
        }

        printf("Peers info updated. ");
        sleep(30);
    }
    return;
}

void main_thread() {
    
}

void listen_thread(in_port_t port) {
    int sockfd, income_sockfd;
    struct sockaddr_in local, income;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[LISTEN] socket()");
        pthread_exit(0);
    }

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *) &local, sizeof(local)) < 0) {
        perror("[LISTEN] bind()");
        pthread_exit(0);
    }

    listen(sockfd, 5);

    puts("[LISTEN] Start to listen to connection");

    while (1) {
        int len = sizeof(income);

        income_sockfd = accept(sockfd, (struct sockaddr*) &income, &len);

        if (income_sockfd < 0) {
            perror("[LISTEN] accept()");
            pthread_exit(0);
        }
// TODO Thread management ?

//        pthread_create(threads + n, NULL, (void * (*) (void *)) handle, 
    }

}

void handle(int sockfd) {
    char cmd[2];
    puts("== New incoming connection handling ==");


    if (read(sockfd, cmd, 2) != 2) {
        puts("Cannot read the command lol");
        pthread_exit(NULL);
    }

    switch (cmd[0]) {
        case 0x02: 
            if (!cmd[1]) {
               puts("\tHandle test request from tracker");
               test_reply(sockfd);
            } else {
                puts("\tHi tracker ... fake me?! ");
            }
            close(sockfd);    
            break;
        case 0x05:
            if (cmd[1] == 1) {
                puts("\tHandle bitmap retrival request");
                peer_bitmap(sockfd);
                break;
            } else {
                puts("\tHi ... You fake me?! ");
                close(sockfd);
            }
        case 0x06: 
            if (cmd[1] == 2) {
                puts("\tHandle chunk request");
                peer_chunk(sockfd);
            } else {
                puts("\tHi ... You fake me again?! ");
                close(sockfd);
            }
        default: 
            puts("\tUnknown command .. Ignore =]");
            close(sockfd);
    }

    pthread_exit(0);
    return;
}
