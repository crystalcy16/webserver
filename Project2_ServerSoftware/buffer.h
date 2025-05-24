#ifndef BUFFER_H
#define BUFFER_H

#include <sys/types.h>

typedef struct {
    int connection;
    off_t file_size;
} request_t;

int enqueue(request_t *buf, int *tail, int *cnt, int buff_size, int conn, off_t size);
request_t dequeue_fifo(request_t *buf, int *head, int *cnt, int buff_size);

#endif
