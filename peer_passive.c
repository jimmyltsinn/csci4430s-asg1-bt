#include "peer.h"

void thread_listen(in_port_t port) {
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
        pthread_t thread;
        struct thread_list_t *thread_entry = malloc(sizeof(struct thread_list_t));
        
        income_sockfd = accept(sockfd, (struct sockaddr*) &income, &len);

        if (income_sockfd < 0) {
            perror("[LISTEN] accept()");
            pthread_exit(0);
        }
    
        thread_entry -> id = thread;
        list_add(&(thread_entry -> list), &(thread_list_head() -> list));
        pthread_create(&thread, NULL, (void * (*) (void *)) handle_main, (void *) income_sockfd);
    }
}

void handle_main(int sockfd) {
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
               handle_trackertest(sockfd);
            } else {
                puts("\tHi tracker ... fake me?! ");
            }
            close(sockfd);    
            break;
        case 0x05:
            if (cmd[1] == 1) {
                puts("\tHandle bitmap retrival request");
                handle_bitmap(sockfd);
                break;
            } else {
                puts("\tHi ... You fake me?! ");
                close(sockfd);
            }
        case 0x06: 
            if (cmd[1] == 2) {
                puts("\tHandle chunk request");
                handle_chunk(sockfd);
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

void handle_trackertest(int sockfd) {
    char msg[2];
    
    fprintf(stderr, "Reply test message to tracker\n"); 
    msg[0] = 0x12;
    msg[1] = 0;

    write(sockfd, msg, 2);

    return;
}

void handle_bitmap(int sockfd) {
    unsigned int status = 1;

    puts("== Received bitmap request ==");
    if (!mode) {
        puts("\tI am not working ...");
        status = 0;
    } else {
        read(sockfd, &status, 4);
        status -= fileid;
    }
    
    if (!status) {
        /* Send the bitmap */
        unsigned int len, tmp;
        char *msg; 
        puts("== Send bitmap ==");

        len = (nchunk + 8) >> 3;
        msg = malloc(2 + 4 + tmp);

        msg[0] = 0x15;
        msg[1] = 1;

        tmp = htonl(len);
        memcpy(msg + 2, &tmp, 4);

        memcpy(msg + 2 + 4, filebitmap, len);

        write(sockfd, msg, 2 + 4 + len);
        free(msg);
    } else {
        /* Reject request */
        char msg[2];    
        puts("== Reject bitmap request ==");
        msg[0] = 0x25;
        msg[1] = 0;

        write(sockfd, msg, 2);
    }

    close(sockfd);
    return;
}

void handle_chunk(int sockfd) {
    puts("== Receive chunk request ==");
    char *msg;
    unsigned int status = 1, offset;

    puts("== Received bitmap request ==");
    if (!bitc_get(mode, 2)) {
        puts("\tI am not working ...");
        status = 0;
    } else {
        read(sockfd, &status, 4);
        status -= fileid;
    }
    
    read(sockfd, &offset, 4);
    offset = htonl(offset);
    if (offset >= filesize)
        status = 1;

    if (!status) {
        unsigned int size, tmp;
        char *msg;
        
        if ((offset + (1 << CHUNK_SIZE)) > filesize) {
            size = filesize - offset;
        } else {
            size = (1 << CHUNK_SIZE);
        }

        msg = malloc(2 + 4 + size);
    
        msg[0] = 0x16;
        msg[1] = 1;

        tmp = htonl(size);
        memcpy(msg + 2, &tmp, 4);
    
        lseek(filefd, offset, SEEK_SET);
        read(filefd, msg + 2 + 4, size);

        write(sockfd, msg, size + 2 + 4);
    } else {
        char msg[2];
        puts("== Reject chunk request ==");
        msg[0] = 0x26;
        msg[1] = 0;

        write(sockfd, msg, 2);
    }

    close(sockfd);
    return; 
}
