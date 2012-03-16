#include <stdio.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    int filesize, i;
    FILE *fd;
    
    if (argc != 3) {
        printf("Usage:\t%s filename size", argv[0]);
        return -1;
    }
    
    filesize = atoi(argv[2]) * 256 * 1024;

//    fd = fopen(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0777);
    fd = fopen(argv[1], "w");
      
        perror("fopen()");

    for (i = 0; i < filesize; ++i) {
        char tmp;
        tmp = rand() % 26 + 'a';
        fwrite(&tmp, 1, 1, fd);
    }
    
    fclose(fd);

    return 0;
}
