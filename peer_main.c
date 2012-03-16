#include "peer.h"

void start() {
    pthread_t tmp;
    tracker_reg();
    if (bitc_get(mode, 1))
        pthread_create(&tmp, NULL, main_thread, NULL);
    pthread_create(&tmp, NULL, thread_keeptrack, NULL);
}

void init() {
    int i;

    mode = 0;
    fileid = 0;
    filesize = 0;
    filename = NULL;
    nchunk = 0;
    filebitmap = NULL;
    bitmap_size = 0;
    dling_peer = 0;
    peers_freq = NULL;
    filefd = -1;

    tracker_ip.s_addr = 0;
    tracker_port = 0;
    local_ip.s_addr = 0;
    local_port = 0;

    pthread_mutex_init(&mutex_filebm, NULL);
    pthread_mutex_init(&mutex_peer, NULL);
    pthread_mutex_init(&mutex_dling, NULL);
    pthread_mutex_init(&mutex_filefd, NULL);

    filename = NULL;

    return;
}

void subseed_promt(char *torrentname) {
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

int reg_torrent(char *torrentname) {
    int fd, i;

    unsigned int tmpl;
    unsigned short tmps;

    fprintf(stderr, "== Register a job ==\n");

    fd = open(torrentname, O_RDONLY);
    if (fd < 0) {
        perror("Open torrent file");
        return -1;
    }
    
    read(fd, &tmpl, 4);    fileid = tmpl;
    read(fd, &tmpl, 4);    tracker_ip.s_addr = htonl(le32toh(tmpl));
    read(fd, &tmps, 2);    tracker_port = htons(le16toh(tmps));
    read(fd, &tmpl, 4);    tmpl = le32toh(tmpl);

    filename = malloc(tmpl + 1);
    read(fd, filename, tmpl);
    filename[tmpl] = '\0';
    
    read(fd, &tmpl, 4);    filesize = le32toh(tmpl);

    close(fd);

    nchunk = off2index(filesize);
    bitmap_size = (nchunk + 8) >> 3;

    return 0;
}

void filefd_init() {
    char flag = 0;

    if (bitc_get(mode, 1) && bitc_get(mode, 2))
        flag |= O_CREAT | O_TRUNC | O_RDWR;    
    else if (bitc_get(mode, 1))
        flag |= O_CREAT | O_TRUNC | O_WRONLY;
    else 
        flag |= O_RDONLY;

    filefd = open(filename, flag, 0644);
    
    if (filefd < 0) {
        perror("Open download / upload file error");
        exit(1);
    }

    return;
}

void bitmap_init() {
    int i;
    filebitmap = malloc(sizeof(char) * bitmap_size);
    for (i = 0; i < PEER_NUMBER; ++i)
        peers_bitmap[i] = malloc(sizeof(char) * bitmap_size);
    peers_freq = malloc(sizeof(int) * nchunk);
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

    for (i = 0; i < PEER_NUMBER; ++i) {
        peers_ip[i].s_addr = 0;
        peers_port[i] = 0;
        memset(peers_bitmap[i], 0, off2index(nchunk));
    }
    puts("All internal variable about peers cleared. ");

    return; 
}

void list() {
    int i;
    puts("== Tracker List ==");
    for (i = 0; i < PEER_NUMBER; ++i)
        printf("\t%s : %d\n", inet_ntoa(peers_ip[i]), peers_port[i]);
    return;
}
