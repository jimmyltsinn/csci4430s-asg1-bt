#include "peer.h"

void thread_download_manager() {
    while (1) {
        int i, cnt;
        if (!bitc_get(mode, 8))
            pthread_exit(NULL);
        for (i = 0; i < PEER_NUMBER; ++i) { 
            if (!peers_ip[i].s_addr)
                continue;
            if (!bitc_get(dling_peer, i)) {
                struct argv_download *argv = malloc(sizeof(struct argv_download));
                pthread_t tmp;
                int index;
                
                pthread_mutex_lock(&mutex_dling);
                bitc_set(dling_peer, i);
                pthread_mutex_unlock(&mutex_dling);
                
                do {
                    index = chunk_list_findfirst(i);
                    if (index < 0)
                        break;
                    chunk_list_del(index, i);
                } while (bit_get(filebitmap, index));

                chunk_list_indexclear(index);

                if (index < 0) {
                    continue;
                }

                argv -> peer = i;
                argv -> index = index;

                pthread_create(&tmp, NULL, (void * (*) (void *)) thread_download_job, argv);
            }
        }
		cnt = 0;
        for (i = 0; i < nchunk; ++i)
            if (bit_get(filebitmap, i))
                ++cnt;
        if (cnt == nchunk) {
			printf("\nDownload complete. \n");
            bitc_reset(mode, 1);
            pthread_exit(0);
        }
        sleep(0);
    }
    return;
}

void thread_track() {
    while (1) {
        int i, j, k;
       
        if (!bitc_get(mode, 8))
            pthread_exit(NULL);
         
        pthread_mutex_lock(&mutex_peer);
        tracker_list();
        
        /* Get bitmap from each peer */
        for (i = 0; i < PEER_NUMBER; ++i) {
            memset(peers_bitmap[i], 0, bitmap_size);
            if ((!peers_ip[i].s_addr) || ((peers_ip[i].s_addr == local_ip.s_addr) && (peers_port[i] == local_port))) {
                bitc_set(dling_peer, i);
                continue;
            }
            getbitmap(i);
        }

        /* Count the frequency */
        memset(peers_freq, 0, sizeof(int) * nchunk);
        for (i = 0; i < nchunk; ++i) {
            if (!bit_get(filebitmap, i)) {
                for (j = 0; j < PEER_NUMBER; ++j) {
                    if (bit_get(peers_bitmap[j], i)) {
                        ++peers_freq[i];
                    }
                }
            } else {
                peers_freq[i] = -1;
            }
        }

        chunk_list_clear();
        for (i = 1; i <= PEER_NUMBER; ++i) {
            for (j = 0; j < nchunk; ++j) {
                if (peers_freq[j] == i) {
                    for (k = 0; k < PEER_NUMBER; ++k) {
                        if (bit_get(peers_bitmap[k], j)) {
                            chunk_list_add(j, k);
                        }
                    }
                }
            }
        }

        dling_peer = 0;
        pthread_mutex_unlock(&mutex_peer);

        sleep(30);
    }
    return;
}

void thread_download_job(void* argv) {
    int peer = ((struct argv_download*) argv) -> peer;
    int index = ((struct argv_download*) argv) -> index;
    pthread_detach(pthread_self());
    getchunk(peer, index << CHUNK_SIZE);
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
        return;
    }

    if (connect(sockfd, (struct sockaddr*) &tgt, sizeof(tgt)) < 0) {
		close(sockfd);
        return;
    }

    /* Sending request of bitmap */
    buf[0] = 0x05;
    buf[1] = 1;
    tmp = htonl(4);
    memcpy(buf + 2, &tmp, 4);
    tmp = htonl(fileid);
    memcpy(buf + 2 + 4, &tmp, 4);
    write(sockfd, buf, 10);

    /* Receive the bitmap ? */
    if (read(sockfd, buf, 2) != 2)
        goto out;
    
    switch (buf[0]) {
        case 0x15: 
            if (buf[1] == 1)
                break;
        case 0x25:
            if (!buf[1]) {
               pthread_mutex_lock(&mutex_dling);
               bitc_set(dling_peer, peerid);
               pthread_mutex_unlock(&mutex_dling);
               goto out;
            }
        default: 
            exit(1);
            goto out;
    }

    if (read(sockfd, &tmp, 4) != 4)
        goto out;

    tmp = ntohl(tmp);
    if (tmp != bitmap_size)
        goto out;

    if (read(sockfd, peers_bitmap[peerid], tmp) != tmp) {
        memset(peers_bitmap[peerid], 0, bitmap_size);
        goto out;
    }

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
    int filefd; 

    memset(&tgt, 0, sizeof(tgt));
    tgt.sin_family = AF_INET;
    tgt.sin_addr = peers_ip[peerid];
    tgt.sin_port = peers_port[peerid];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return;
	}

    if (connect(sockfd, (struct sockaddr*) &tgt, sizeof(tgt)) < 0) {
        close(sockfd);
		return;
	}

    /* Send request of chunk */
    buf[0] = 0x06;
    buf[1] = 2;
    tmp = htonl(4);
    memcpy(buf + 2, &tmp, 4);
    tmp = htonl(fileid);
    memcpy(buf + 2 + 4, &tmp, 4);
    tmp = htonl(4);
    memcpy(buf + 2 + 4 + 4, &tmp, 4);
    tmp = htonl(offset);
    memcpy(buf + 2 + 4 + 4 + 4, &tmp, 4);
    write(sockfd, buf, 18);
   
    /* Receive the chunk ? */
    if (recv(sockfd, buf, 2, 0) != 2) {
        goto out;
	}
    
    switch (buf[0]) {
        case 0x16: 
            if (buf[1] == 1) {
                goto write;
            } else {
                goto out;
            }
        case 0x26:
            if (!buf[1]) {
                goto out;
            }
        default:
            goto out;
    }

write: 
    if (read(sockfd, &tmp, 4) != 4) 
        goto out;

    tmp = ntohl(tmp);
    data = malloc(tmp);
   
    if (recvn(sockfd, data, tmp) != tmp) {
		free(data);
        goto out;
	}

    pthread_mutex_lock(&mutex_filefd);
    
	filefd = open(filename, O_WRONLY);
    lseek(filefd, offset, SEEK_SET);
    write(filefd, data, tmp);
    
    close(filefd);

    pthread_mutex_unlock(&mutex_filefd);

	pthread_mutex_lock(&mutex_filebm);
    bit_set(filebitmap, off2index(offset));
    pthread_mutex_unlock(&mutex_filebm);

    free(data);

out: 
    pthread_mutex_lock(&mutex_dling);
    bitc_reset(dling_peer, peerid);
    pthread_mutex_unlock(&mutex_dling);

    close(sockfd);
    return;
}
