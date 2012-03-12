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
#include "list.h"

#define CHUNK_SIZE 18
#define PEER_NUMBER 5

#define bit_set(s, pos) s[pos >> 3] |= 1 << (pos & 7)
#define bit_get(s, pos) (s[pos >> 3] & (1 << (pos & 7)))

#define bitc_set(c, pos) c |= 1 << (pos & 7)
#define bitc_get(c, pos) (c & (1 << (pos & 7)))

#define off2index(offset) ((offset + (1 << CHUNK_SIZE)) >> CHUNK_SIZE)

struct thread_list_t {
    struct list_head list;
    pthread_t id;
};

/* Global variable */
unsigned int fileid;
unsigned int filesize;
char *filename;
unsigned int nchunk;
char *filebitmap;
int filefd;

char mode; /* using 2 bits to represent ... 1st bit is download and 2nd bit is upload */

pthread_mutex_t mutex_finished, mutex_downloading, mutex_peers; 
int *peers_freq;

/* The IPs and ports are in network byte ordering */
struct in_addr tracker_ip, local_ip;
unsigned short tracker_port, local_port; 

struct in_addr peers_ip[PEER_NUMBER];
unsigned short peers_port[PEER_NUMBER];
char *peers_bitmap[PEER_NUMBER];

/********** FUNCTIONS *********/
/* sort.c 
   Code from Spring 2012 CSCI2100B+S*/
void sort(int n, int *a);

/* peer_base.c */
struct thread_list_t *thread_list_head();

/* peer_tracker.c */
int tracker_reg();
int tracker_unreg();
//int tracker_list(); //TODO Unused ??

/* peer_passive.c */
void thread_listen(in_port_t port);
void handle_main(int sockfd);
void handle_trackertest(int sockfd);
void handle_bitmap(int sockfd);
void handle_chunk(int sockfd);

/* peer_accept.c */
void thread_keeptrack();
void getbitmap(int peerid);
void getchunk(int peerid, int offset);

/* peer_main.c */
int start(char *torrentname);
void stop(); 


/* peer_basic.c 
int read_torrent(char *torrentname);
void subseed_init(char *torrent);
struct thread_list_t* thread_list_head();

/* peer_file.c 
//int read_torrent(char *torrentname);

/* peer_tracker.c 
int tracker_reg();
int tracker_unreg();
int tracker_list();
void test_reply(int sockfd);

/* peer_peer.c 
void peer_bitmap(int sockfd);
void peer_bitmap_ask(int sockfd);
void peer_bitmap_send(int sockfd);
void peer_bitmap_reject(int sockfd);
void peer_bitmap_receive(int sockfd, char *buf, int size);
void peer_getbitmap(int peerid);
void peer_chunk(int sockfd);
void peer_chunk_ask(int sockfd, int offset);
void peer_chunk_send(int sockfd, int offset);
void peer_chunk_reject(int sockfd);
void peer_chunk_receive(int sockfd, int offset);

/* peer_main.c 
void main_thread();
void listen_thread(in_port_t sockfd);
void handle(int sockfd);


/* peer.c 
int main(int argc, char **argv) ;*/
