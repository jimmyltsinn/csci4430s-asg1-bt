#include "peer.h"

void main_thread() {

}

static void start_subseed(char *torrentname) {
    printf("\tThere are %d chunks in %s\n", nchunk, torrentname);
    printf("\tWhich chunks to upload? (start counting from 0)\n");

    do {
        char tmp[13], *ptr;
        int range[2];
        int i;
        printf("\t[type . to stop] >> ");
        fflush(stdout);
        fgets(tmp, 13, stdin);
        if (tmp[0] == '.' || tmp[0] == '\n')
            break;
        ptr = tmp;
        range[0] = atoi(strtok_r(tmp, "-", &ptr));
        range[1] = atoi(ptr);
        if (range[0] > range[1] || range[0] > (nchunk - 1) || range[1] > (nchunk - 1)) {
            puts("\tInvalid range");
            continue;
        }
        fprintf(stderr, "%d to %d\n", range[0], range[1]);
        for (i = range[0]; i <= range[1]; ++i) {
            bit_set(filebitmap, i);
        }
    } while (1);
}

int start(char *torrentname) {
    int fd;

    unsigned int tmpl;
    unsigned short tmps;

    fprintf(stderr, "== Add ==\n");

    fd = open(torrentname, O_RDONLY);
    if (fd < 0) {
        perror("Open torrent file");
        return -1;
    }
    
    read(fd, &tmpl, 4);
    fileid = tmpl;

    read(fd, &tmpl, 4);
    tracker_ip.s_addr = htonl(le32toh(tmpl));

    read(fd, &tmps, 2);
    tracker_port = htons(le16toh(tmps));
    
    read(fd, &tmpl, 4);
    tmpl = le32toh(tmpl);
    filename = malloc(tmpl + 1);
    read(fd, filename, tmpl);
    filename[tmpl] = '\0';
    
    read(fd, &tmpl, 4);
    filesize = le32toh(tmpl);

    close(fd);

    filefd = open(filename, O_RDWR | O_CREAT);
    if (filefd < 0) {
        perror("Open target file");
        return -1;
    }

    nchunk = off2index(filesize);
    filebitmap = malloc(sizeof(char) * ((nchunk + 8) >> 3));
    switch (mode) {
        case 1: /* Download */
            memset(filebitmap, 0, (nchunk + 8) >> 3);
            break;
        case 2: /* Normal Seed */
            memset(filebitmap, 0xFF, (nchunk + 8) >> 3);
            break;
        case 3: /* Subseed */
            start_subseed(torrentname);
            break;
    }

    peers_freq = malloc(sizeof(int) * nchunk);

    return 0;
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
