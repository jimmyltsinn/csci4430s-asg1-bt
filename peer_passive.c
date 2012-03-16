#include "peer.h"

void socket_reuse(int fd) {
    long val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1) {
        perror("setsockopt()");
        exit(1);
    }
}

void thread_listen() {
    int sockfd, income_sockfd;
    struct sockaddr_in local, income;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[LISTEN] socket()");
        pthread_exit(0);
    }

    socket_reuse(sockfd);

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(local_port);

    if (bind(sockfd, (struct sockaddr *) &local, sizeof(local)) < 0) {
        perror("[LISTEN] bind()");
        pthread_exit(0);
    }

    listen(sockfd, PEER_NUMBER + 1);

    puts("[LISTEN] Start to listen to connection");

    while (1) {
        int len = sizeof(income);
        pthread_t thread;
        struct thread_list_t *thread_entry = malloc(sizeof(struct thread_list_t));

        puts("[!!!!!] accept() block ...");        
        income_sockfd = accept(sockfd, (struct sockaddr*) &income, &len);
        puts("[!!!!!] accept() return ...");

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
    printf("== New incoming connection handling ==\nfd = %d\n", sockfd);

    pthread_detach(pthread_self());

    if (read(sockfd, cmd, 2) != 2) {
        puts("Cannot read the command lol");
        pthread_exit(NULL);
    }

    switch (cmd[0]) {
        case 0x02: 
            if (!cmd[1]) {
               puts("\t[Handle] Test request from tracker");
               handle_trackertest(sockfd);
            } else {
                puts("\t[Handle] Hi tracker ... fake me?! ");
                close(sockfd);
            }
            break;
        case 0x05:
            if (cmd[1] == 1) {
                puts("\t[Handle] Bitmap retrival request");
                handle_bitmap(sockfd);
            } else {
                puts("\t[Handle] Hi ... You fake me?! ");
                close(sockfd);
            }
            break;
        case 0x06: 
            if (cmd[1] == 2) {
                puts("\t[Handle] Chunk request");
                handle_chunk(sockfd);
            } else {
                puts("\t[Handle] Hi ... You fake me again?! ");
                close(sockfd);
            }
            break;
        default: 
            puts("\t[Handle] Unknown command .. Ignore =]");
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

    close(sockfd);
    return;
}

void handle_bitmap(int sockfd) {
    unsigned int tmp, len;
    char *msg;

    puts("== Received bitmap request ==");
    if (!mode) {
        puts("\tI am not working ...");
        goto reject;
    }

    puts("[SendBitmap] Check argl");    
    read(sockfd, &tmp, 4);
    if (ntohl(tmp) - 4)
        goto reject;
    
    puts("[SendBitmap] Check fileid");
    read(sockfd, &tmp, 4);
    if (tmp - fileid)
        goto reject;
      
    /* Send the bitmap */
    puts("== Send bitmap ==");

    len = (nchunk + 8) >> 3;
    msg = malloc(2 + 4 + len);

    msg[0] = 0x15;
    msg[1] = 1;

    tmp = htonl(len);
    memcpy(msg + 2, &tmp, 4);
    
    printf("[SendBitmap] Get the lock ...");
    pthread_mutex_lock(&mutex_filebm);
    memcpy(msg + 2 + 4, filebitmap, len);
    pthread_mutex_unlock(&mutex_filebm);
    printf("[SendBitmap] Release the log ...");

    write(sockfd, msg, 2 + 4 + len);
    goto out; 

reject: 
    /* Reject request */
    msg = malloc(2);
    puts("== Reject bitmap request ==");
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

    puts("== Receive chunk request ==");
    
    do {
        struct sockaddr_in tgt;
        int tmp = sizeof(tgt);
        getpeername(sockfd, (struct sockaddr*) &tgt, &tmp);
        printf("[!] %s : %d (%d)\n", inet_ntoa(tgt.sin_addr), ntohs(tgt.sin_port), sockfd);
    } while (0);

    if (!bitc_get(mode, 2)) {
        puts("\t[SendChunk] I do not upload ... ");
        goto reject;
    }

    read(sockfd, &tmp, 4);
    if (ntohl(tmp) - 4) {
        puts("\t[SendChunk] Wrong length a");
        goto reject;
    }

    read(sockfd, &tmp, 4);
    if (tmp - fileid) {
        puts("\t[SendChunk] Wrong fileid");
        goto reject;
    }

    read(sockfd, &tmp, 4);
    if (ntohl(tmp) - 4) {
        puts("[SendChunk] Wrong length b");
        goto reject;
    }

    read(sockfd, &offset, 4);
    offset = ntohl(offset);
    if (offset >= filesize) {
        puts("[SendChunk] Over offset");
        goto reject;
    }
    
    /* Send chunk */
    printf("-- Send chunk %d --\n", offset);
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

// TODO Offset calculation confirm  
    pthread_mutex_lock(&mutex_filefd);
    printf("[SendChunk] Using fd = %d\n", filefd);      
    lseek(filefd, offset, SEEK_SET);
    printf("[SendChunk] Open() ...\n");
    read(filefd, msg + 2 + 4, size);
    puts("[SendChunk] Open return");
    pthread_mutex_unlock(&mutex_filefd);
    write(sockfd, msg, size + 2 + 4);
    puts("[SendChunk] Okay, send");
    goto out;

reject: 
    /* Reject chunk request */
    msg = malloc(2);
    puts("-- Reject chunk request --");
    msg[0] = 0x26;
    msg[1] = 0;
    
    write(sockfd, msg, 2);

out:
    printf("[SendChunk] Close the fd\n");
    free(msg);
    close(sockfd);
    return; 
}
