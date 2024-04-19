#include <string.h>

#include "queue.h"

void queueInit(Queue* q) {
	q->head = q->tail = &q->buffer[0];
}

size_t queueWrite(Queue* q, const void* data, size_t length) {
	if (q->head + length > &q->buffer[QUEUESIZE]) {
		if (length > QUEUESIZE) length = QUEUESIZE;
		size_t len1 = &q->buffer[QUEUESIZE] - q->head;
		len1 = queueWrite(q, data, len1);
		return len1 + queueWrite(q, (const unsigned char*)data + len1, length - len1);
	}
	if (q->tail > q->head && q->head + length >= q->tail) {
		length = q->tail - 1 - q->head;
	}
	if (length > 0) {
		memcpy(q->head, data, length);
		q->head += length;
		if (q->head >= &q->buffer[QUEUESIZE]) {
			q->head = &q->buffer[0];
		}
	}
	return length;
}

size_t queueRead(Queue* q, void* buffer, size_t length) {
	if (q->tail + length > &q->buffer[QUEUESIZE]) {
		if (length > QUEUESIZE) length = QUEUESIZE;
		size_t len1 = &q->buffer[QUEUESIZE] - q->tail;
		len1 = queueRead(q, buffer, len1);
		return len1 + queueRead(q, (char *)buffer + len1, length - len1);
	}
	if (q->head >= q->tail && q->tail + length > q->head) {
		length = q->head - q->tail;
	}
	if (length > 0) {
		memcpy(buffer, q->tail, length);
		q->tail += length;
		if (q->tail >= &q->buffer[QUEUESIZE]) {
			q->tail = &q->buffer[0];
		}
	}
	return length;
}
