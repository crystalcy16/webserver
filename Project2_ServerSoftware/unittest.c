#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "request.h"
#include "buffer.h"



// enqueue-dequeue
void test_enqueue_dequeue() {
    request_t buffer[4];
    int head = 0, tail = 0, cnt = 0;
    int size = 4;

    assert(enqueue(buffer, &tail, &cnt, size, 10, 1000) == 0);
    assert(cnt == 1);
    assert(tail == 1);

    request_t r = dequeue_fifo(buffer, &head, &cnt, size);
    assert(r.connection == 10);
    assert(r.file_size == 1000);
    assert(head == 1);
    assert(cnt == 0);

    printf("test_enqueue_dequeue passed\n");
}

// Test 2: Circular wraparound
void test_circular() {
    request_t buffer[2];
    int head = 0, tail = 0, cnt = 0;
    int size = 2;

    enqueue(buffer, &tail, &cnt, size, 1, 111);
    enqueue(buffer, &tail, &cnt, size, 2, 222);
    assert(tail == 0); //wrap
    assert(cnt == 2);

    request_t r1 = dequeue_fifo(buffer, &head, &cnt, size);
    assert(r1.connection == 1);
    assert(head == 1);
    assert(cnt == 1);

    request_t r2 = dequeue_fifo(buffer, &head, &cnt, size);
    assert(r2.connection == 2);
    assert(head == 0); //wrap
    assert(cnt == 0);

    printf("test_circular passed\n");
}

// Test 3: Buffer full
void test_buffer_full() {
    request_t buffer[1];
    int head = 0, tail = 0, cnt = 0;
    int size = 1;

    assert(enqueue(buffer, &tail, &cnt, size, 5, 555) == 0);
    assert(enqueue(buffer, &tail, &cnt, size, 6, 666) == -1); //full

    request_t r = dequeue_fifo(buffer, &head, &cnt, size);
    assert(r.connection == 5);

    printf("test_buffer passed\n");
}

int main() {
    test_enqueue_dequeue();
    test_circular();
    test_buffer_full();
    printf("All tests passed!\n");
    return 0;
}
