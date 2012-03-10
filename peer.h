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
unsigned int fileID;
unsigned int fileSize;
unsigned int nChunk;
char *fileBitmap;
int filefd;
int mode;

/* peer_basic.c */
int init_job(char *filename);
void bit_set(char *s, int pos);
void subseed_init(char *torrent);

/* peer_file.c */
int torrent_file(char *filename);

/* peer_tracker.c */
int tracker_reg(struct in_addr tip, in_port_t tport, in_port_t listen_port);
int tracker_unreg(struct in_addr tip, in_port_t tport, in_port_t listen_port);
int tracker_list(struct in_addr tip, in_port_t tport);
void test_reply(int sockfd);

/* peer.c */
int main(int argc, char **argv);
