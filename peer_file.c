#include "peer.h"

int read_torrent(char *torrentname) {
    int fd;

    unsigned int tmpl;
    unsigned short tmps;

    fd = open(torrentname, O_RDONLY);
    if (fd < 0) {
        perror("Open torrent file");
        return -1;
    }
    
    read(fd, &tmpl, 4);
    fileid = tmpl;

    read(fd, &tmpl, 4);
    tracker_ip.s_addr = tmpl;

    read(fd, &tmps, 2);
    tracker_port = tmps;
    
    read(fd, &tmpl, 4);
    tmpl = le32toh(tmpl);
    filename = malloc(tmpl + 1);
    read(fd, filename, tmpl);
    filename[tmpl] = '\0';
    
    read(fd, &tmpl, 4);
    filesize = le32toh(tmpl);

    close(fd);

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
