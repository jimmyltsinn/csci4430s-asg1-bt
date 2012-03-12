#include "peer.h"

struct thread_list_t* thread_list_head() {
    static struct thread_list_t *head = NULL;
    if (!head) {
        malloc(sizeof(struct thread_list_t));
        INIT_LIST_HEAD(&head -> list);
        head -> id = -1;
    }
    return head;
}


