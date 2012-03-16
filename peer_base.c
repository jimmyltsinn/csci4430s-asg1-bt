#include "peer.h"

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
    if (!thread_list_find(id)) return;
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

void chunk_list_clear() {
    struct chunk_list_t *tgt, *save;
    list_for_each_entry_safe(tgt, save, &(chunk_list_head() -> list), list)
        list_del(&tgt -> list);
    return;
}

int chunk_list_cnt() {
    int ret = 0;
    struct chunk_list_t *tgt;
    list_for_each_entry(tgt, &chunk_list_head() -> list, list)
        ++ret;
    return ret;
}
