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

#pragma once

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

// How much free space for writing.
size_t queueWriteAvailable(Queue* q);

// How many bytes available for reading.
size_t queueReadAvailable(Queue* q);
