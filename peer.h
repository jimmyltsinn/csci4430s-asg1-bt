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
#define QUEUE_SIZE 40

#define bit_set(s, pos) s[pos >> 3] |= 1 << pos
#define bit_get(s, pos) (s[pos >> 3] & (1 << pos & 7))

#define bitc_set(c, pos) c |= 1 << pos
#define bitc_reset(c, pos) c &= ~ (1 << pos)
#define bitc_get(c, pos) (c & (1 << pos))

#define off2index(offset) ((offset + (1 << CHUNK_SIZE)) >> CHUNK_SIZE)

struct thread_list_t {
    struct list_head list;
    pthread_t id;
};

struct chunk_list_t {
    struct list_head list;
    int offset;
    int peer; 
};

struct argv_download {
    int peer;
    int offset;
};

/* Global variable */
unsigned int fileid;
unsigned int filesize;
char *filename;
unsigned int nchunk;
char *filebitmap;
int filefd;

char mode; /* using 2 bits to represent ... 1st bit is download and 2nd bit is upload */

pthread_mutex_t mutex_bitmap, mutex_list, mutex_dling;
char dling_peer;
//pthread_mutex_t mutex_finished, mutex_downloading, mutex_peers, mutex_work;
int *peers_freq;

/* The IPs and ports are in network byte ordering */
struct in_addr tracker_ip, local_ip;
unsigned short tracker_port, local_port; 
int download_queue[QUEUE_SIZE];

struct in_addr peers_ip[PEER_NUMBER];
unsigned short peers_port[PEER_NUMBER];
char *peers_bitmap[PEER_NUMBER];

/********** FUNCTIONS *********/
/* sort.c 
   Code from Spring 2012 CSCI2100B+S*/
void sort(int n, int *a);

/* peer_base.c */
struct thread_list_t* thread_list_head();
struct thread_list_t* thread_list_find(pthread_t id);
void thread_list_add(pthread_t id);
void thread_list_del(pthread_t id);

struct chunk_list_t* chunk_list_head();
struct chunk_list_t* chunk_list_find(int offset, int peer);
void chunk_list_add(int offset, int peer);
void chunk_list_del(int offset, int peer);
int chunk_list_findfirst(int peer);
void chunk_list_clear();

/* peer_tracker.c */
int tracker_reg();
int tracker_unreg();
//int tracker_list(); //TODO Unused ??

/* peer_passive.c */
void thread_listen();
void handle_main(int sockfd);
void handle_trackertest(int sockfd);
void handle_bitmap(int sockfd);
void handle_chunk(int sockfd);

/* peer_accept.c */
void thread_keeptrack();
void getbitmap(int peerid);
void getchunk(int peerid, int offset);

/* peer_main.c */
void init();
void thread_keeptrack();
void thread_download(void* argv);
void thread_main_download();
void main_thread();
int add_job(char *torrent);
void start();
void stop(); 

