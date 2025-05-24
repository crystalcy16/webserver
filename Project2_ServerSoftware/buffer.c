#include "buffer.h"

int enqueue(request_t *buf, int *tail, int *cnt, int buff_size, int conn, off_t size) {
    if (*cnt == buff_size) return -1;
    buf[*tail].connection = conn;
    buf[*tail].file_size = size;
    *tail = (*tail + 1) % buff_size;
    (*cnt)++;
    return 0;
}

request_t dequeue_fifo(request_t *buf, int *head, int *cnt, int buff_size) {
    request_t req = buf[*head];
    *head = (*head + 1) % buff_size;
    (*cnt)--;
    return req;
}
