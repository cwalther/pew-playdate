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

#include "terminal.h"
#include "globals.h"

#include <stdint.h>

#define WIDTH_CHARS 50
#define HEIGHT_CHARS 16
#define CELLW 8
#define CELLH 15

static uint16_t buffer[WIDTH_CHARS*HEIGHT_CHARS];
static uint16_t* cursor = &buffer[0];
static int utf8shift = 0;
static LCDFont* font = NULL;
static LCDFont* font1 = NULL;
static LCDFont* font2 = NULL;
static int dirtyRowsBegin = 0;
static int dirtyRowsEnd = 0;
static int cursorx = 0;
static int cursory = 0;
static unsigned int lastActivity = 0;
static int blink = 0;
static enum { ESEQ_NONE, ESEQ_ESC, ESEQ_ESC_BRACKET, ESEQ_ESC_BRACKET_DIGIT } eseqstate = ESEQ_NONE;
static int eseqn = 0;

void terminalInit(PlaydateAPI* pd) {
	const char* err;
	font = font1 = pd->graphics->loadFont("fonts/Moof-15", &err);
	if (font1 == NULL) {
		pd->system->error("Couldn't load terminal font: %s", err);
	}
	font2 = pd->graphics->loadFont("fonts/Gnu-15", &err);
	if (font2 == NULL) {
		pd->system->error("Couldn't load terminal font: %s", err);
	}
}

void terminalTouch(void) {
	dirtyRowsBegin = 0;
	dirtyRowsEnd = HEIGHT_CHARS;
}

void terminalPutchar(unsigned char c) {
	int cursorjumped = 0;
	if (eseqstate == ESEQ_NONE) {
		// Decode UTF-8 to UCS-2. Invalid UTF-8 is not detected but treated as
		// garbage-in-garbage-out.
		if ((c & 0x80) == 0) {
			// single-byte UTF-8
			switch (c) {
				case '\x08': // backspace
					if (cursor != &buffer[0]) {
						cursor--;
					}
					break;
				case '\x1b': // esc
					eseqstate = ESEQ_ESC;
					break;
				case '\n':
					cursor += WIDTH_CHARS;
					cursorjumped = 1;
					break;
				case '\r':
					cursor = &buffer[0] + ((cursor - &buffer[0]) / WIDTH_CHARS) * WIDTH_CHARS;
					break;
				default:
					*cursor++ = c;
					break;
			}
		}
		else if ((c & 0x40) == 0) {
			// multi-byte UTF-8 continuation
			*cursor |= ((c & 0x3f) << utf8shift);
			if (utf8shift == 0) {
				cursor++;
			}
			else {
				utf8shift -= 6;
			}
		}
		else if ((c & 0x20) == 0) {
			// 2-byte UTF-8 start
			*cursor = ((c & 0x1f) << 6);
			utf8shift = 0;
		}
		else if ((c & 0x10) == 0) {
			// 3-byte UTF-8 start
			*cursor = ((c & 0x0f) << 12);
			utf8shift = 6;
		}
		else if ((c & 0x08) == 0) {
			// 4-byte UTF-8 start - we can't represent those in uint16_t, for
			// now just drop the overflowing bits
			*cursor = 0; // ((c & 0x07) << 18);
			utf8shift = 12;
		}
		else {
			// invalid UTF-8
			*cursor++ = 0xFFFD; // REPLACEMENT CHARACTER
		}
	}
	else if (eseqstate == ESEQ_ESC) {
		switch(c) {
			case '[':
				eseqn = 0;
				eseqstate = ESEQ_ESC_BRACKET;
				break;
			default:
				eseqstate = ESEQ_NONE;
				break;
		}
	}
	else if (eseqstate == ESEQ_ESC_BRACKET || eseqstate == ESEQ_ESC_BRACKET_DIGIT) {
		if ('0' <= c && c <= '9') {
			eseqn = eseqn*10 + (c - '0');
			eseqstate = ESEQ_ESC_BRACKET_DIGIT;
		}
		else {
			if (c == 'A') { // cursor up
				if (eseqstate == ESEQ_ESC_BRACKET) eseqn = 1;
				for (; eseqn > 0; eseqn--) {
					if (cursor >= buffer + WIDTH_CHARS) {
						cursor -= WIDTH_CHARS;
						cursorjumped = 1;
					}
				}
			}
			else if (c == 'B') { // cursor down
				if (eseqstate == ESEQ_ESC_BRACKET) eseqn = 1;
				for (; eseqn > 0; eseqn--) {
					if (cursor < buffer + WIDTH_CHARS*(HEIGHT_CHARS - 1)) {
						cursor += WIDTH_CHARS;
						cursorjumped = 1;
					}
				}
			}
			else if (c == 'C') { // cursor forward
				if (eseqstate == ESEQ_ESC_BRACKET) eseqn = 1;
				for (; eseqn > 0; eseqn--) {
					if (((cursor - &buffer[0]) % WIDTH_CHARS) < WIDTH_CHARS - 1) {
						cursor += 1;
						cursorjumped = 1;
					}
				}
			}
			else if (c == 'D') { // cursor back
				if (eseqstate == ESEQ_ESC_BRACKET) eseqn = 1;
				for (; eseqn > 0; eseqn--) {
					if (((cursor - &buffer[0]) % WIDTH_CHARS) > 0) {
						cursor -= 1;
					}
				}
			}
			else if (c == 'K') { // erase in line
				if (eseqstate == ESEQ_ESC_BRACKET) eseqn = 0;
				uint16_t* begin;
				uint16_t* end;
				switch (eseqn) {
					case 0: // to end of line
						begin = cursor;
						end = &buffer[0] + ((cursor - &buffer[0]) / WIDTH_CHARS + 1) * WIDTH_CHARS;
						break;
					case 1: // to beginning of line
						begin = &buffer[0] + ((cursor - &buffer[0]) / WIDTH_CHARS) * WIDTH_CHARS;
						end = cursor;
						break;
					case 2: // entire line
						begin = &buffer[0] + ((cursor - &buffer[0]) / WIDTH_CHARS) * WIDTH_CHARS;
						end = begin + WIDTH_CHARS;
						break;
					default: // invalid
						begin = cursor;
						end = cursor;
						break;
				}
				for (uint16_t* p = begin; p < end; p++) {
					*p = (p < cursor) ? ' ' : '\0';
				}
			}
			eseqstate = ESEQ_NONE;
		}
	}

	if (cursor >= buffer + sizeof(buffer)/sizeof(buffer[0])) {
		// scroll
		memmove(buffer, buffer + WIDTH_CHARS, WIDTH_CHARS*(HEIGHT_CHARS - 1)*sizeof(buffer[0]));
		memset(buffer + WIDTH_CHARS*(HEIGHT_CHARS - 1), 0, WIDTH_CHARS*sizeof(buffer[0]));
		cursor -= WIDTH_CHARS;
		cursorjumped = 1;
		dirtyRowsBegin = 0;
		dirtyRowsEnd = HEIGHT_CHARS;
	}
	else {
		int row = (cursor - &buffer[0]) / WIDTH_CHARS;
		if (dirtyRowsBegin == dirtyRowsEnd) {
			dirtyRowsBegin = row;
			dirtyRowsEnd = row + 1;
		}
		else {
			if (dirtyRowsBegin > row) dirtyRowsBegin = row;
			row++;
			if (dirtyRowsEnd < row) dirtyRowsBegin = row;
		}
	}
	if (cursorjumped) {
		// drawText() will stop at zeros so replace any that it should not stop
		// at by spaces
		uint16_t* rowstart = &buffer[0] + ((cursor - &buffer[0]) / WIDTH_CHARS) * WIDTH_CHARS;
		for (uint16_t* p = cursor - 1; p >= rowstart; p--) {
			if (*p == 0) *p = ' ';
		}
	}
	lastActivity = global_pd->system->getCurrentTimeMilliseconds();
}

void terminalWrite(const char* data, size_t len) {
	const char* end = data + len;
	for (const char* p = data; p != end; p++) {
		terminalPutchar(*p);
	}
}

void terminalUpdate(PlaydateAPI* pd) {
	// Until I can make up my mind about which font looks better: switch fonts
	// using the A button.
	PDButtons pushed;
	pd->system->getButtonState(NULL, &pushed, NULL);
	if (pushed & kButtonA) {
		font = (font == font1) ? font2 : font1;
		terminalTouch();
	}

	if (dirtyRowsEnd != dirtyRowsBegin) {
		int cursorrow = (cursor - &buffer[0]) / WIDTH_CHARS;
		if (dirtyRowsBegin <= cursorrow && cursorrow < dirtyRowsEnd) {
			// text update erases the cursor
			blink = 0;
		}

		int y = CELLH*dirtyRowsBegin;
		pd->graphics->fillRect(0, y, 400, CELLH*(dirtyRowsEnd - dirtyRowsBegin), kColorWhite);
		pd->graphics->setFont(font);
		for (int i = dirtyRowsBegin; i < dirtyRowsEnd; i++, y += CELLH) {
			pd->graphics->drawText(&buffer[i*WIDTH_CHARS], WIDTH_CHARS*sizeof(buffer[0]), k16BitLEEncoding, 1, y);
		}
		dirtyRowsBegin = dirtyRowsEnd = 0;
	}

	unsigned int now = pd->system->getCurrentTimeMilliseconds();
	int nblink = ((int)(now - lastActivity) < 500) || ((now & (1 << 9)) != 0);
	if (nblink != blink) {
		if (nblink) {
			// draw the cursor at the new location
			cursorx = ((cursor - &buffer[0]) % WIDTH_CHARS)*CELLW + 1;
			cursory = ((cursor - &buffer[0]) / WIDTH_CHARS)*CELLH;
		}
		// else erase it at the old location
		pd->graphics->fillRect(cursorx, cursory, CELLW, CELLH, kColorXOR);
		blink = nblink;
	}
}
