#include "terminal.h"

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
static int blink = 0;

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
	// Decode UTF-8 to UCS-2. Invalid UTF-8 is not detected but treated as
	// garbage-in-garbage-out.
	if ((c & 0x80) == 0) {
		// single-byte UTF-8
		switch (c) {
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

	int nblink = ((pd->system->getCurrentTimeMilliseconds() & (1 << 9)) != 0);
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
