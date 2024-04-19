#include <stddef.h>

#define QUEUESIZE 1016

typedef struct {
	unsigned char* head;
	unsigned char* tail;
	unsigned char buffer[QUEUESIZE];
} Queue;

void queueInit(Queue* q);

// Write at most `length` bytes from `data` into the queue. Returns number of
// bytes written. If that is fewer than requested, the queue is full.
size_t queueWrite(Queue* q, const void* data, size_t length);

// Read at most `length` bytes from the queue to `buffer`. Returns number of
// bytes read. If that is fewer than requested, the queue is empty.
size_t queueRead(Queue* q, void* buffer, size_t length);
