#include "peer.h"

int init_job(char *filename) {
    int fd;
    unsigned int ip, id, lfn, size; 
    unsigned short port;
   
    char *fn;

    fprintf(stderr, "== Add ==\n");

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Open file error");
        return -1;
    }

    read(fd, &id, 4);
    
    read(fd, &ip, 4);
    ip = le32toh(ip);

    read(fd, &port, 2);
    port = le16toh(port);

    read(fd, &lfn, 4);
    lfn = le32toh(lfn);

    fn = malloc(sizeof(char) * (lfn + 1));
    read(fd, fn, lfn);
    fn[lfn] = '\0';

    read(fd, &size, 4);
    size = le32toh(size);
    
    close(fd);
    
    fileID = id;
    fileSize = size;
    
    filefd = open(fn, O_RDWR | O_CREAT);
    if (filefd < 0) {
        perror("Open target file");
        return -1;
    }

    nChunk = (size >> 18);
    fileBitmap = malloc(sizeof(char) * (nChunk + 8) >> 3);
    switch (mode) {
        case 1: /* Download */
            memset(fileBitmap, 0, (nChunk + 8) >> 3);
            break;
        case 2: /* Normal Seed */
            memset(fileBitmap, 0xFF, (nChunk + 8) >> 3);
            break;
        case 3: /* Subseed */
            subseed_init(filename);
            break;
    }


    return 0;
}

void bit_set(char *s, int pos) {
    s[pos >> 3] |= 1 << (pos & 7);
    return;
}

void subseed_init(char *torrent) {
    printf("\tThere are %d chunks in %s\n", nChunk, torrent);
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
        if (range[0] > range[1] || range[0] > (nChunk - 1) || range[1] > (nChunk - 1)) {
            puts("\tInvalid range");
            continue;
        }
        fprintf(stderr, "%d to %d\n", range[0], range[1]);
        for (i = range[0]; i <= range[1]; ++i) {
            bit_set(fileBitmap, i);
        }
    } while (1);
}
