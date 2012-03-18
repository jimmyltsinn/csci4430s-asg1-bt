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
#include <signal.h>
#include "list.h"

#define CMD_TYPE 11
#define CHUNK_SIZE 18
#define PEER_NUMBER 5

#define bit_set(s, pos) s[pos >> 3] |= 1 << (pos & 7)
#define bit_get(s, pos) (s[pos >> 3] & (1 << (pos & 7)))

#define bitc_set(c, pos) c |= 1 << (pos & 7)
#define bitc_reset(c, pos) c &= ~ (1 << (pos & 7))
#define bitc_get(c, pos) (c & (1 << (pos & 7)))

#define off2index(offset) ((offset) >> CHUNK_SIZE)

struct thread_list_t {
    struct list_head list;
    pthread_t id;
};

struct chunk_list_t {
    struct list_head list;
    int index;
    int peer; 
};

struct argv_download {
    int peer;
    int index;
};

/* Global variable */
unsigned int fileid;
unsigned int filesize;
char *filename;
unsigned int nchunk;
char *filebitmap;
unsigned int bitmap_size;
pthread_t main_thread;
//unsigned char fileflag;
/* 8 bit for current mode 
 * 	1st bit		download
 * 	2nd bit		upload
 * 	8th bit		work
 */
char mode;

//pthread_mutex_t mutex_bitmap, mutex_list, mutex_dling;

pthread_mutex_t mutex_filebm, mutex_peer, mutex_dling, mutex_filefd;
char dling_peer;
int *peers_freq;

/* The IPs and ports are in network byte ordering */
struct in_addr tracker_ip, local_ip;
unsigned short tracker_port, local_port; 

struct in_addr peers_ip[PEER_NUMBER];
unsigned short peers_port[PEER_NUMBER];
char *peers_bitmap[PEER_NUMBER];

/********** FUNCTIONS *********/
/* peer_base.c */
void socket_reuse(int fd);
size_t recvn(int sockfd, void* buf, size_t len);
size_t sendn(int sockfd, const void *buf, size_t cnt);

struct thread_list_t *thread_list_head();
struct thread_list_t *thread_list_find(pthread_t id);
void thread_list_add(pthread_t id);
void thread_list_del(pthread_t id);

struct chunk_list_t *chunk_list_head();
struct chunk_list_t *chunk_list_find(int index, int peer);
int chunk_list_findfirst(int peer);
void chunk_list_add(int index, int peer);
void chunk_list_del(int index, int peer);
void chunk_list_clear();
void chunk_list_indexclear(int index);
int chunk_list_cnt();

/* peer_cmd.c */
void help();
//void bitmap_print(char *bitmap);
int reg_torrent(char *torrent);
void filefd_init();
void bitmap_init();
void subseed_promt(char *torrent);
void start();
void stop();
void info();
void list();
void progress();

/* peer_active.c */
void thread_download_manager();
void thread_track();
void thread_download_job(void *argv);
void getbitmap(int peerid);
void getchunk(int peerid, int offset);

/* peer_passive.c */
void thread_listen();
void handle_main(int sockfd);
void handle_trackertest(int sockfd);
void handle_bitmap(int sockfd);
void handle_chunk(int sockfd); 

/* peer_tracker.c */
int tracker_reg();
int tracker_unreg();
int tracker_list();
