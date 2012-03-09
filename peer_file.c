#include "peer.h"

int torrent_file(char *filename) {
    int fd;

    unsigned int id, size, lfn;
    char tip[4];
    unsigned short tport;
    char *fn;

    fd = open(filename, O_RDONLY);
    
    read(fd, &id, 4);
    
    read(fd, &tip, 4);
    *tip = le32toh((uint32_t) *tip);

    read(fd, &tport, 2);
    tport = le16toh((uint16_t) tport);
    
    read(fd, &lfn, 4);
    lfn = le32toh((uint32_t) lfn);
    fn = malloc(lfn + 1);
    read(fd, fn, lfn);
    fn[lfn] = '\0';
    
    read(fd, &size, 4);
    size = le32toh((uint32_t) size);

    close(fd);

    printf("ID = %x\n", id);
    printf("Tracker IP: %d.%d.%d.%d\n", tip[0], tip[1], tip[2], tip[3]);
    printf("Tracker Port: %d\n", tport);
    printf("File name (%d) : %s\n", lfn, fn);
    printf("File size: %u\n", size);
    return 0;
}
/*
int main(int argc, char **argv) {
    if (argc > 1)
        return torrent_file(argv[1]);
    
    printf("Usage: %s filename\n", argv[0]);
    return -1;
}
*/
