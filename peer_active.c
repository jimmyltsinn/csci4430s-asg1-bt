#include "peer.h"

void main_thread() {
    while (1) {
        int i;
        printf("\t\tMODE = %d\n", mode);
        for (i = 0; i < PEER_NUMBER; ++i) {        
            if (!peers_ip[i].s_addr)
                continue;
            if (!bitc_get(dling_peer, i)) {
                struct argv_download *argv = malloc(sizeof(struct argv_download));
                pthread_t tmp;
                int offset;
                
                pthread_mutex_lock(&mutex_dling);
                bitc_set(dling_peer, i);
                pthread_mutex_unlock(&mutex_dling);
                
                do {
                    offset = chunk_list_findfirst(i);
                    if (offset < 0)
                        break;
                    chunk_list_del(offset, i);
                } while (bit_get(filebitmap, offset));

                if (offset < 0) 
                    continue;
                argv -> peer = i;
                argv -> offset = offset;

                pthread_create(&tmp, NULL, (void * (*) (void *)) thread_download, argv);
            }
        }
        sleep(1);
    }
    return;
}

void thread_keeptrack() {
    while (1) {
        int i, j, k;
        
        pthread_mutex_lock(&mutex_list);
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

        chunk_list_clear();
        for (j = 0; j < PEER_NUMBER; ++j)
            for (i = 0; i < nchunk; ++i) 
                if (peers_freq[i] == j) 
                    for (k = 0; k < PEER_NUMBER; ++k) 
                        if (bit_get(filebitmap, i)) {
                            printf("[Track] %d has chunk %d\n", k, i);
                            chunk_list_add(j, nchunk << 3);
                        }
            
        pthread_mutex_unlock(&mutex_list);
        printf("Peers info updated. ");
        sleep(30);
    }
    return;
}

void thread_main_download() {
    
}

void thread_download(void* argv) {
    int peer = ((struct argv_download*) argv) -> peer;
    int offset = ((struct argv_download*) argv) -> offset;
    getchunk(peer, offset);
    pthread_mutex_lock(&mutex_dling);
    bitc_reset(dling_peer, peer);
    pthread_mutex_unlock(&mutex_dling);
    free(argv);
    pthread_exit(0);
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
               pthread_mutex_lock(&mutex_dling);
               bitc_set(dling_peer, peerid);
               pthread_mutex_unlock(&mutex_dling);
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

    pthread_mutex_lock(&mutex_bitmap);
    bit_set(filebitmap, off2index(offset));
    pthread_mutex_unlock(&mutex_bitmap);

    free(data);

out: 
    close(sockfd);
    return;
}
