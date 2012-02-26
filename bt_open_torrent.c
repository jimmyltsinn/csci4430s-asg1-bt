#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <endian.h>
#include <stdint.h>

int torrent_file(char *filename) {
    int fd;

    unsigned int id, size;
    char tip[4];
    unsigned short tport;
    char *name;

    fd = open(filename, O_RDONLY);
    
    read(fd, &id, 4);
    read(fd, &tip, 4);
    read(fd, &tport, 2);
    
    read(fd, &size, 4);
    name = malloc(size + 1);
    read(fd, name, size);
    name[size] = '\0';
    
    *tip = le32toh((uint32_t) *tip);

    read(fd, &size, 4);
    close(fd);

    printf("ID = %x\n", id);
    printf("Tracker IP: %d.%d.%d.%d\n", tip[0], tip[1], tip[2], tip[3]);
    printf("Tracker Port: %d\n", tport);
    printf("File name: %s\n", name);
    printf("File size: %u\n", size);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1)
        return torrent_file(argv[1]);
    return -1;
}