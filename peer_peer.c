#include "peer.h"

int selecting(int sockfd) {
    char buf[2];

    read(sockfd, buf, 2);
    
    switch (buf[0]) {
        case 0x06: 
            /* Receive request of chunk */
            peer_chunk(sockfd);
            break;
        case 0x16: 
            /* Receive chunk */
            peer_chunk_receive(sockfd, 0);
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

    puts("== Ask for bitmap ==");

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

    puts("== Send bitmap ==");

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
    
    puts("== Reject bitmap request ==");

    msg[0] = 0x25;
    msg[1] = 0;

    write(sockfd, msg, 2);
    
    close(sockfd);
    return;
}

void peer_bitmap_receive(int sockfd, char *buf, int size) {
    unsigned int len;
    unsigned int tmp;
    char *bitmap;

    puts("== Receive a bitmap ==");

    read(sockfd, &tmp, 4);
    len = ntohl(tmp);

    if (size < len) {
        puts("Length not enough ... Somethings wrong");
        return;
    }

    read(sockfd, buf, len);

    return;
}

void peer_chunk(int sockfd) {
    puts("== Receive chunk request ==");
    if (!bitc_get(mode, 2)) {
        puts("\tDownload only =D");
        peer_chunk_reject(sockfd);
        return;
    }

//TODO Chunk offset    
    peer_chunk_send(sockfd, 0);
    
    return;
}

void peer_chunk_ask(int sockfd, int offset) {
    char msg[18];
    unsigned int tmp;

    printf("== Ask for chunk %d ==\n", offset);

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

void peer_chunk_send(int sockfd, int offset) {
    unsigned int size;
    char *data;
    unsigned int tmp;
    char *msg;

    puts("== Send a chunk ==");
    
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

    close(sockfd);
    return;
}

void peer_chunk_reject(int sockfd) {
    char msg[2];
    
    puts("== Reject chunk request ==");

    msg[0] = 0x26;
    msg[1] = 0;

    write(sockfd, msg, 2);

    close(sockfd);
    
    return;
}

void peer_chunk_receive(int sockfd, int offset) {
    unsigned int tmp;
    unsigned int size;
    char *data;
    
    puts("== Receive a chunk ==");

    read(sockfd, &tmp, 4);

    size = ntohl(tmp);
    data = malloc(size);

    read(sockfd, data, size);
    close(sockfd);
    
    lseek(filefd, offset, SEEK_SET);
    write(filefd, data, size);

    bit_set(filebitmap, off2index(offset));

    return;
}
