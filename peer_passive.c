#include "peer.h"

void thread_listen() {
    int sockfd, income_sockfd;
    struct sockaddr_in local, income;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Listening socket");
        exit(1);
    }

    socket_reuse(sockfd);

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = local_port;

    if (bind(sockfd, (struct sockaddr *) &local, sizeof(local)) < 0) {
        perror("Listening port");
        exit(1);
    }

    listen(sockfd, PEER_NUMBER + 1);

    while (1) {
        int len = sizeof(income);
        pthread_t thread;
        struct thread_list_t *thread_entry = malloc(sizeof(struct thread_list_t));

        income_sockfd = accept(sockfd, (struct sockaddr*) &income, &len);

        if (income_sockfd < 0)
            continue;
    
        thread_entry -> id = thread;
        pthread_create(&thread, NULL, (void * (*) (void *)) handle_main, (void *) income_sockfd);
    }
}

void handle_main(int sockfd) {
    char cmd[2];

    pthread_detach(pthread_self());

    if (read(sockfd, cmd, 2) != 2) {
        pthread_exit(NULL);
    }

    switch (cmd[0]) {
        case 0x02: 
            if (!cmd[1]) {
               handle_trackertest(sockfd);
            } else {
                close(sockfd);
            }
            break;
        case 0x05:
            if (cmd[1] == 1) {
                handle_bitmap(sockfd);
            } else {
                close(sockfd);
            }
            break;
        case 0x06: 
            if (cmd[1] == 2) {
                handle_chunk(sockfd);
            } else {
                close(sockfd);
            }
            break;
        default: 
            close(sockfd);
    }

    pthread_exit(0);
    return;
}

void handle_trackertest(int sockfd) {
    char msg[2];
    
    msg[0] = 0x12;
    msg[1] = 0;

    write(sockfd, msg, 2);

    close(sockfd);
    return;
}

void handle_bitmap(int sockfd) {
    unsigned int tmp, len;
    char *msg;

    if (!bitc_get(mode, 8))             goto reject;
    if (read(sockfd, &tmp, 4) != 4)     goto reject;
    if (ntohl(tmp) - 4)                 goto reject;
    if (read(sockfd, &tmp, 4) != 4)     goto reject;
    if (tmp - ntohl(fileid))            goto reject;

    len = (nchunk + 8) >> 3;
    msg = malloc(2 + 4 + len);

    msg[0] = 0x15;
    msg[1] = 1;

    tmp = htonl(len);
    memcpy(msg + 2, &tmp, 4);
    
    pthread_mutex_lock(&mutex_filebm);
    memcpy(msg + 2 + 4, filebitmap, len);
    pthread_mutex_unlock(&mutex_filebm);

    write(sockfd, msg, 2 + 4 + len);
    goto out; 

reject: 
    /* Reject request */
    msg = malloc(2);
    msg[0] = 0x25;
    msg[1] = 0;

    write(sockfd, msg, 2);

out: 
    free(msg);
    close(sockfd);
    return;
}

void handle_chunk(int sockfd) {
    char *msg;
    unsigned int offset, tmp, size;
    int filefd;

    if (!bitc_get(mode, 2))             goto reject;
    if (read(sockfd, &tmp, 4) != 4)     goto reject;
    if (ntohl(tmp) - 4)                 goto reject;
    if (read(sockfd, &tmp, 4) != 4)     goto reject;
    if (tmp - ntohl(fileid))            goto reject;
    if (read(sockfd, &tmp, 4) != 4)     goto reject;
    if (ntohl(tmp) - 4)                 goto reject;
    if (read(sockfd, &offset, 4) != 4)  goto reject; 
    offset = ntohl(offset);
    if (offset >= filesize)             goto reject;
    
    /* Send chunk */
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

    pthread_mutex_lock(&mutex_filefd);
    filefd = open(filename, O_RDONLY);
    lseek(filefd, offset, SEEK_SET);
    if (read(filefd, msg + 2 + 4, size) != size) {
        pthread_mutex_unlock(&mutex_filefd);
        free(msg);
        goto reject;
    }
    close(filefd);
    pthread_mutex_unlock(&mutex_filefd);
    
    sendn(sockfd, msg, size + 2 + 4);

    goto out;

reject: 
    /* Reject chunk request */
    msg = malloc(2);
    msg[0] = 0x26;
    msg[1] = 0;
    
    write(sockfd, msg, 2);

out:
    free(msg);
    close(sockfd);
    return; 
}
