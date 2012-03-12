#include "peer.h"

void thread_keeptrack() {
    while (1) {
        int i, j;
        
        pthread_mutex_lock(&mutex_peers);
        tracker_list();
        for (i = 0; i < PEER_NUMBER; ++i) {
            memset(peers_bitmap[i], 0, (nchunk + 8) >> 3);
            if (!peers_ip[i].s_addr)
                continue;
            getbitmap(i);
        }
        memset(peers_freq, 0, sizeof(int) * nchunk);
        for (i = 0; i < nchunk; ++i)
            if (!bit_get(filebitmap, i))
                for (j = 0; j < nchunk; ++j)
                    if (bit_get(peers_bitmap[j], i))
                        ++peers_freq[i];
            else
                peers_freq[i] = -1;
        pthread_mutex_unlock(&mutex_peers);
        printf("Peers info updated. ");
        sleep(30);
    }
    return;
}

void getbitmap(int peerid) {
    struct sockaddr_in tgt;
    int sockfd; 
    char buf[10];
    unsigned int tmp;

    memset(&tgt, 0, sizeof(tgt));
    tgt.sin_family = AF_INET;
    tgt.sin_addr = peers_ip[peerid];
    tgt.sin_port = peers_port[peerid];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[P_GETBM] socket()");
        return;
    }

    if (connect(sockfd, (struct sockaddr*) &tgt, sizeof(tgt)) < 0) {
        perror("[P_GETBM] connect()");
        return;
    }

    /* Sending request of bitmap */
    buf[0] = 0x05;
    buf[1] = 1;
    tmp = htonl(4);
    memcpy(buf + 2, &tmp, 4);
    tmp = fileid;
    memcpy(buf + 2 + 4, &tmp, 4);
    write(sockfd, buf, 10);

    /* Receive the bitmap ? */
    read(sockfd, buf, 2);
    switch (buf[0]) {
        case 0x15: 
            if (buf[1] == 1)
                break;
        case 0x25:
            if (!buf[1]) {
               puts("[P_GETBM] Cannot get the bitmap");
               goto out;
            }
        default: 
            puts("[P_GETBM] Unknown command .. ");
            goto out;
    }

    read(sockfd, &tmp, 4);
    tmp = ntohl(tmp);

    read(sockfd, peers_bitmap[peerid], tmp);

out: 
    close(sockfd);
    return;
}

void getchunk(int peerid, int offset) {
    struct sockaddr_in tgt;
    unsigned int tmp;
    int sockfd; 
    char buf[18];
    char *data;

    memset(&tgt, 0, sizeof(tgt));
    tgt.sin_family = AF_INET;
    tgt.sin_addr = peers_ip[peerid];
    tgt.sin_port = peers_port[peerid];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[P_GETCHUNK] socket()");
        return;
    }

    if (connect(sockfd, (struct sockaddr*) &tgt, sizeof(tgt)) < 0) {
        perror("[P_GETCHUNK] connect()");
        return;
    }

    /* Send request of chunk */
    buf[0] = 0x06;
    buf[1] = 2;
    tmp = htonl(4);
    memcpy(buf + 2, &tmp, 4);
    tmp = fileid;
    memcpy(buf + 2 + 4, &tmp, 4);
    tmp = htonl(4);
    memcpy(buf + 2 + 4 + 4, &tmp, 4);
    tmp = htonl(offset);
    memcpy(buf + 2 + 4 + 4 + 4, &tmp, 4);
    write(sockfd, buf, 18);
   
    /* Receive the chunk ? */
    read(sockfd, buf, 2);
    switch (buf[0]) {
        case 0x16: 
            if (buf[1] == 1) {
                goto write;
                puts("GOTO cannot in switch .. =D");
                break;
            }
        case 0x26:
            if (!buf[1]) {
                puts("Fail to receive chunk. ");
                goto out;
            }
        default:
            puts("Unknown message recevied ...");
            goto out;
    }

write: 
    read(sockfd, &tmp, 4);
    tmp = ntohl(tmp);

    data = malloc(tmp);

    read(sockfd, data, tmp);
    
    write(filefd, data, tmp);

    pthread_mutex_lock(&mutex_finished);
    bit_set(filebitmap, off2index(offset));
    pthread_mutex_unlock(&mutex_finished);

    free(data);

out: 
    close(sockfd);
    return;
}
