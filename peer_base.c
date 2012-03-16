#include "peer.h"

/* By Mole */
void socket_reuse(int fd) {
    long val = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1) {
        perror("setsockopt()");
        exit(1);
    }
}

/* By Ma MingCao, from tracker.c */
size_t recvn (int sockfd, void *buf, size_t len) {
    fd_set rfds;
    struct timeval tv;
    int retval;
    int read_len = 0;
    char *ptr = (char*) buf;

    while (read_len < len) {
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        retval = select(sockfd+1, &rfds, NULL, NULL, &tv);
        if (retval == -1) {
            perror("select()");
            return 0;
        } else if (retval) {
            int l;
            l = read(sockfd,ptr,len - read_len);
            if (l > 0) {
                ptr += l;
                read_len+=l;
            } else if (l <= 0) {
                return read_len;
            }
        } else {
            printf("No data within 2 seconds.\n");
            return 0;
        }
    }
    return read_len;
}

size_t sendn(int sockfd, const void *buf, size_t cnt) {
    int ret = 0;
    do {
        int tmp;
        tmp = write(sockfd, buf + ret, cnt - ret);
        if (tmp < 0) {
            perror("sendn()");
            return ret;
        }
        ret += tmp;
        printf("[sendn] Written %d [%d out of %d]\n", tmp, ret, cnt);
    } while (ret < cnt);
    return ret;
}

struct thread_list_t* thread_list_head() {
    static struct thread_list_t *head = NULL;
    if (!head) {
        head = malloc(sizeof(struct thread_list_t));
        INIT_LIST_HEAD(&head -> list);
        head -> id = 0;
    }
    return head;
}

struct thread_list_t* thread_list_find(pthread_t id) {
    struct thread_list_t *tmp;
    list_for_each_entry(tmp, &thread_list_head() -> list, list) {
        if (tmp -> id == id)
            return tmp;
    }
    return 0;
}

void thread_list_add(pthread_t id) {
    struct thread_list_t *tmp;
    if (thread_list_find(id)) return;
    tmp = malloc(sizeof(struct thread_list_t));
    tmp -> id = id;
    list_add(&tmp -> list, &thread_list_head() -> list);
    return;
}

void thread_list_del(pthread_t id) {
    struct thread_list_t *tmp;
    tmp = thread_list_head(id);
    if (!tmp) return;
    tmp -> id = -1;
    list_del(&tmp -> list);
    free(tmp);
    return;
}

struct chunk_list_t *chunk_list_head() {
    static struct chunk_list_t *head = NULL;
    if (!head) {
        head = malloc(sizeof(struct chunk_list_t));
        INIT_LIST_HEAD(&head -> list);
        head -> index = -1;
        head -> peer = 0;
    }   
    return head;
}

struct chunk_list_t* chunk_list_find(int index, int peer) {
    struct chunk_list_t *tmp;
    list_for_each_entry(tmp, &chunk_list_head() -> list, list)
        if (tmp -> index == index)
            if (tmp -> peer == peer)
                return tmp;
    return NULL;
}

int chunk_list_findfirst(int peer) {
    struct chunk_list_t *tmp;
    list_for_each_entry(tmp, &chunk_list_head() -> list, list)
        if (tmp -> peer == peer)
            return tmp -> index;
    return -1;
}

void chunk_list_add(int index, int peer) {
    struct chunk_list_t *tmp; 
    tmp = malloc(sizeof(struct chunk_list_t));
    tmp -> index = index;
    tmp -> peer = peer;
    list_add_tail(&tmp -> list, &chunk_list_head() -> list);
    return;
}

void chunk_list_del(int index, int peer) {
    struct chunk_list_t *tgt;
    tgt = chunk_list_find(index, peer);
    if (!tgt)
        return;
    list_del(&tgt -> list);
    tgt -> index = -1;
    tgt -> peer = -1;
    free(tgt);
    return;
}

void chunk_list_indexclear(int index) {
    struct chunk_list_t *tgt, *save;
    list_for_each_entry_safe(tgt, save, &(chunk_list_head() -> list), list)
        if (tgt -> index == index) {
            list_del(&tgt -> list);
            tgt -> index = -1;
            tgt -> peer = -1;
            free(tgt);
        }
}

void chunk_list_clear() {
    struct chunk_list_t *tgt, *save;
    list_for_each_entry_safe(tgt, save, &(chunk_list_head() -> list), list) {
        list_del(&tgt -> list);
        tgt -> index = -1;
        tgt -> peer = -1;
        free(tgt);
    }
    return;
}

int chunk_list_cnt() {
    int ret = 0;
    struct chunk_list_t *tgt;
    list_for_each_entry(tgt, &chunk_list_head() -> list, list)
        ++ret;
    return ret;
}
