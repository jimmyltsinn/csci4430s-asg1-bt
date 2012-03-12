#include "peer.h"

void tracking_thread() {
    while (1) {
        int i, j;
        
        pthread_mutex_lock(&mutex_peers);
        tracker_list();
        for (i = 0; i < PEER_NUMBER; ++i) {
            memset(peers_bitmap[i], 0, (nchunk + 8) >> 3);
            if (!peers_ip[i].s_addr)
                continue;
            peer_getbitmap(i);
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

void main_thread() {

}

void peer_getbitmap(int peerid) {
    struct sockaddr_in tgt;
    int sockfd; 
    char buf[2];

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

    peer_bitmap_ask(sockfd);

    read(sockfd, buf, 2);
    
    switch (buf[0]) {
        case 0x15: 
            if (buf[1] == 1) {
                peer_bitmap_receive(sockfd, peers_bitmap[peerid], (nchunk + 8) >> 3);
                break;
            }
        case 0x25:
            if (!buf[1]) {
               puts("[P_GETBM] Cannot get the bitmap");
               break;
            }
        default: 
            puts("[P_GETBM] Unknown command .. ");
    }

    close(sockfd);
}

void peer_getchunk(int peerid, int offset) {
    struct sockaddr_in tgt;
    unsigned int size;
    int sockfd; 
    char buf[2];
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

    peer_chunk_ask(sockfd, offset);
    
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
    read(sockfd, &size, 4);
    size = ntohl(size);

    data = malloc(size);

    read(sockfd, data, size);
    
    write(filefd, data, size);

    pthread_mutex_lock(&mutex_finished);
    bit_set(filebitmap, off2index(offset));
    pthread_mutex_unlock(&mutex_finished);

    free(data);

out: 
    close(sockfd);
    return;
}

void stop() {
    int i;
    struct thread_list_t *tmp, *s;

    list_for_each_entry_safe(tmp, s, &(thread_list_head() -> list), list) {
        if (pthread_kill(tmp -> id, 2)) {
            perror("pthread_kill()");
        } else {
            list_del(&tmp -> list);
            free(tmp);
        }
    }
    puts("All registered thread are KILLED =D");

    pthread_mutex_trylock(&mutex_finished);
    pthread_mutex_unlock(&mutex_finished);
    pthread_mutex_trylock(&mutex_downloading);
    pthread_mutex_unlock(&mutex_downloading);
    pthread_mutex_trylock(&mutex_peers);
    pthread_mutex_unlock(&mutex_peers);
    puts("All mutex unlocked =D");

    for (i = 0; i < PEER_NUMBER; ++i) {
        peers_ip[i].s_addr = 0;
        peers_port[i] = 0;
        memset(peers_bitmap[i], 0, off2index(nchunk));
    }
    puts("All internal variable about peers cleared. ");

    return; 
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
        pthread_t thread;
        struct thread_list_t *thread_entry = malloc(sizeof(struct thread_list_t));
        
        income_sockfd = accept(sockfd, (struct sockaddr*) &income, &len);

        if (income_sockfd < 0) {
            perror("[LISTEN] accept()");
            pthread_exit(0);
        }
    
        thread_entry -> id = thread;
        list_add(&(thread_entry -> list), &(thread_list_head() -> list));
        pthread_create(&thread, NULL, (void * (*) (void *)) handle, (void *) income_sockfd);
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
