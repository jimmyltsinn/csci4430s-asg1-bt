#include "peer.h"

static void bitmap_print(char *bitmap) {
    int i, j;

    for (i = 0; i < nchunk; ++i) {
        if (i % 48 == 0) printf("\n\t");
        else if (i % 8 == 0) printf(" ");
        printf("%1d", bit_get(bitmap, i) ? 1 : 0);
    }
    
    printf("\n");
}

void help() {
    printf("The following commands are avalible: \n");
    if (!mode) {
        printf("\tdown [filename]   \tAdd a new download ONLY job\n");
        printf("\tadd [filename]    \tAdd a new download job\n");
        printf("\tseed [filename]   \tAdd a new seed job\n");
        printf("\tsubseed [filename]\tAdd a subseed job\n");
    } else {
        printf("\tinfo              \tPrint the information of current job\n");
        printf("\tstop              \tStop the current job\n");
        printf("\tresume            \tResume a stopped job\n");
        printf("\tprogress          \tShow the progress of current downloading job\n");
        printf("\tpeer              \tPrint the IP address and port of peers of current downloading job\n");
    }
    printf("\thelp              \tShow this manual\n");
    printf("\texit              \tExit\n");
    return; 
}

int reg_torrent(char *torrentname) {
    int fd, i;

    unsigned int tmpl;
    unsigned short tmps;

//    fprintf(stderr, "== Register a job ==\n");

    fd = open(torrentname, O_RDONLY);
    if (fd < 0) {
//        perror("Open torrent file");
        return -1;
    }
    
    read(fd, &tmpl, 4);    fileid = le32toh(tmpl);
    read(fd, &tmpl, 4);    tracker_ip.s_addr = htonl(le32toh(tmpl));
    read(fd, &tmps, 2);    tracker_port = htons(le16toh(tmps));
    read(fd, &tmpl, 4);    tmpl = le32toh(tmpl);

    filename = malloc(tmpl + 1);
    read(fd, filename, tmpl);
    filename[tmpl] = '\0';
    
    read(fd, &tmpl, 4);    filesize = le32toh(tmpl);

    close(fd);

    nchunk = (filesize + (1 << CHUNK_SIZE) - 1) >> CHUNK_SIZE;
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
        perror("Open target file");
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

void sighandler(int sig) {
    if (sig == SIGINT)
        pthread_exit(NULL);
}

void start() {
    pthread_t tmp;
    if (tracker_reg()) 
        exit(1);

    signal(SIGINT, sighandler);

    if (bitc_get(mode, 1)) {
        pthread_create(&tmp, NULL, (void * (*) (void *)) thread_download_manager, NULL);
    }
    thread_list_add(tmp);

    pthread_create(&tmp, NULL, (void * (*) (void *)) thread_track, NULL);
    thread_list_add(tmp);
    return;
}

void stop() {
    int i;
    struct thread_list_t *tmp, *s;

    list_for_each_entry_safe(tmp, s, &(thread_list_head() -> list), list) {
        printf("Waiting for join ... 0x%lx \n", tmp -> id);
        pthread_kill(tmp -> id, SIGINT);
        pthread_join(tmp -> id, NULL);
        list_del(&tmp -> list);
        free(tmp);
    }
//    puts("All registered thread are KILLED =D");

    for (i = 0; i < PEER_NUMBER; ++i) {
        peers_ip[i].s_addr = 0;
        peers_port[i] = 0;
        memset(peers_bitmap[i], 0, off2index(nchunk));
    }
//    puts("All internal variable about peers cleared. ");

    tracker_unreg();

    return; 
}

void info() {
    printf("== Current job info ==\n");
    printf("\tLocal IP: %s:%d\n", inet_ntoa(local_ip), ntohs(local_port));
    printf("\tTracker IP: %s:%d\n", inet_ntoa(tracker_ip), ntohs(tracker_port));

    printf("\n");

    printf("\tFile ID: 0x%x\n", fileid);
    printf("\tFile name: %s\n", filename ? filename : "(Unknown)");
    printf("\tFile size: %d\n", filesize);
    printf("\tNumber of chunk: %d\n", nchunk);
    printf("\tNumber of byte for bitmap: %d\n", bitmap_size);

    printf("\tDownload ? %s\n", bitc_get(mode, 1) ? "YES" : "no");
    printf("\tUpload ? %s\n", bitc_get(mode, 2) ? "YES" : "no");

    return;
}

void list() {
    int i;
    puts("== Peer List ==");
    for (i = 0; i < PEER_NUMBER; ++i)
        if (peers_ip[i].s_addr)
            printf("%s\t%s : %d\n", ((peers_ip[i].s_addr == local_ip.s_addr && peers_port[i] == htons(local_port)) ? "Local >" : ""), inet_ntoa(peers_ip[i]), peers_port[i]);
    return;
}

void progress() {
    int i, cnt;
    double percent;

    cnt = 0;

    for (i = 0; i < nchunk; ++i) 
        if (bit_get(filebitmap, i))
            ++cnt;
   
    percent = (double) cnt / (nchunk) * 100;

    printf("== Progress ==\n");
    printf("\tFile Bitmap");
    bitmap_print(filebitmap);
    
    printf("\tNumber of chunk finished = %d\n", cnt);
    printf("\t%f %c has been completed. \n", percent, '%');

    return;
}

