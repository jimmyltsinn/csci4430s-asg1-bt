#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <endian.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

/* Global variable */
unsigned int fileid;
unsigned int filesize;
char *filename;
unsigned int nchunk;
char *filebitmap;
int filefd;
int mode;

/* The IPs and ports are in network byte ordering */
struct in_addr tracker_ip, local_ip;
unsigned short tracker_port, local_port; 

/* peer_basic.c */
int init_job(char *filename);
void bit_set(char *s, int pos);
void subseed_init(char *torrent);

/* peer_file.c */
int read_torrent(char *torrentname);

/* peer_tracker.c */
int tracker_reg();
int tracker_unreg();
int tracker_list();
void test_reply(int sockfd);

/* peer.c */
int main(int argc, char **argv);
