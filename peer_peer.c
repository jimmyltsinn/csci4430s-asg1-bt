#include "peer.h"

int selecting(int sockfd) {
    char buf[2];

    read(sockfd, buf, 2);
    
    switch (buf[0]) {
        case 0x05: 
            /* Receive request of Bitmap */
            peer_bitmap(sockfd);
            break;
        case 0x15: 
            /* Receive a bitmap */
            peer_bitmap_receive(sockfd);
            break;
        case 0x25: 
            /* Could not receive a bitmap */
            puts("Could not receive its bitmap ..");
            break;
        case 0x06: 
            /* Receive request of chunk */
            peer_chunk(sockfd);
            break;
        case 0x16: 
            /* Receive chunk */
            peer_chunk_receive(sockfd);
            break;
        case 0x26:
            /* Cannot receive a chunk */
            puts("Could not receive a chunk ..");
            break;
        default: 
            puts("Receiving undefined peer request ... Just ignore it. ");
    }
    return 0;    
}

void peer_bitmap(int sockfd) {
    puts("== Received bitmap request ==");
    unsigned int id;

    if (!mode) {
        puts("\tI am not working ...");
        peer_bitmap_reject(sockfd);
        return;
    }
    
    read(sockfd, &id, 4);
    
    if (id != fileid) {
        puts("\tI am not downloading this ...");
        peer_bitmap_reject(sockfd);
        return;
    }

    peer_bitmap_send(sockfd);
    return;
}

void peer_bitmap_ask(int sockfd) {
    char msg[10];
    unsigned int tmp;

    msg[0] = 0x05;
    msg[1] = 1;

    tmp = htonl(4);
    memcpy(msg + 2, &tmp, 4);

    tmp = fileid;
    memcpy(msg + 2 + 4, &tmp, 4);

    write(sockfd, msg, 10);

    return;
}

void peer_bitmap_send(int sockfd) {
    char *msg;
    unsigned int len;
    unsigned int tmp;

    len = (nchunk + 8) >> 3;
    msg = malloc(2 + 4 + len);

    msg[0] = 0x15;
    msg[1] = 1;

    tmp = htonl(len);
    memcpy(msg + 2, &tmp, 4);

    memcpy(msg + 2 + 4, filebitmap, len);

    write(sockfd, msg, 2 + 4 + len);

    close(sockfd);
    free(msg);
    return;
}

void peer_bitmap_reject(int sockfd) {
    char msg[2];
    
    msg[0] = 0x25;
    msg[1] = 0;

    write(sockfd, msg, 2);
    
    close(sockfd);
    return;
}

void peer_bitmap_receive(int sockfd) {
    unsigned int len;
    unsigned int tmp;
    char *bitmap;

    read(sockfd, &tmp, 4);
    len = ntohl(tmp);

    bitmap = malloc(len);
    read(sockfd, bitmap, len);

    close(sockfd);

    //TODO How to process the bitmap ...
    return;
}

void peer_chunk(int sockfd) {
    puts("== Receive chunk request ==");
    if (!bitc_get(mode, 2)) {
        puts("\tDownload only =D");
        peer_chunk_reject(sockfd);
        return;
    }
    
    peer_chunk_send(sockfd);
    return;
}

void peer_chunk_ask(int sockfd, int offset) {
    char msg[18];
    unsigned int tmp;

    msg[0] = 0x06;
    msg[1] = 2;

    tmp = htonl(4);
    memcpy(msg + 2, &tmp, 4);

    tmp = fileid;
    memcpy(msg + 2 + 4, &tmp, 4);

    tmp = htonl(4);
    memcpy(msg + 2 + 4 + 4, &tmp, 4);

    tmp = htonl(offset);
    memcpy(msg + 2 + 4 + 4 + 4, &tmp, 4);

    write(sockfd, msg, 18);
    return;
}

void peer_chunk_send(int sockfd) {
    


    return;
}

void peer_chunk_reject(int sockfd) {
    char msg[2];
    
    msg[0] = 0x26;
    msg[1] = 0;

    write(sockfd, msg, 2);

    close(sockfd);
    
    return;
}

void peer_chunk_receive(int sockfd) {
    
    return;
}
