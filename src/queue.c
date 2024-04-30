/*
Copyright (c) 2024 Christian Walther

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

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

size_t queueWriteAvailable(Queue* q) {
	ptrdiff_t diff = q->tail - q->head;
	if (diff < 0) diff += QUEUESIZE;
	return diff - 1;
}

size_t queueReadAvailable(Queue* q) {
	ptrdiff_t diff = q->head - q->tail;
	if (diff < 0) diff += QUEUESIZE;
	return diff;
}
