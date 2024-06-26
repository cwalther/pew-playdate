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

#include "display.h"
#include "globals.h"
#include "preferences.h"
#include "terminal.h"

#include "py/mphal.h"
#include "py/binary.h"

#include <stdint.h>
#include <string.h>

#define WIDTH 8
#define HEIGHT 8
#define TILEW 29
#define TILEH 29
#define MX (200 - (WIDTH/2)*TILEW)
#define MY (120 - (HEIGHT/2)*TILEH)

enum {
	eDirtyBackground = 1 << 0
};
enum {
	eIndicatorMenu = WIDTH*HEIGHT,
	eIndicatorA,
	eBufferSize
};

static LCDBitmapTable* pixeltable;
static LCDBitmap* background;
static struct Indicator {
	const char* tableName;
	int x;
	int y;
	LCDBitmapTable* table;
} indicators[3] = {
	{"images/indicatorMenu", 390, 45},
	{"images/indicatorA", 374, 229},
	{NULL},
};
static uint8_t frontbuf[eBufferSize];
static uint8_t backbuf[eBufferSize];
static uint8_t dirty;
static PDButtons currentKeys;
static PDButtons collectedKeys;

void displaySetInverted(PlaydateAPI* pd, int inv) {
	const char* err;
	pd->graphics->freeBitmapTable(pixeltable);
	pixeltable = pd->graphics->loadBitmapTable(inv ? "images/pixels-inv" : "images/pixels", &err);
	if (pixeltable == NULL) {
		pd->system->error("Couldn't load bitmap table: %s", err);
	}
	pd->graphics->freeBitmap(background);
	background = pd->graphics->loadBitmap(inv ? "images/background-inv" : "images/background", &err);
	if (background == NULL) {
		pd->system->error("Couldn't load bitmap: %s", err);
	}
	dirty = eDirtyBackground;
}

void displayInit(PlaydateAPI* pd) {
	const char* err;
	for (struct Indicator* i = &indicators[0]; i->tableName != NULL; i++) {
		i->table = pd->graphics->loadBitmapTable(i->tableName, &err);
		if (i->table == NULL) {
			pd->system->error("Couldn't load bitmap table: %s", err);
		}
	}
	displaySetInverted(pd, preferences.inverted);
	memset(frontbuf, 255, eBufferSize);
}

void displayTouch(void) {
	dirty |= eDirtyBackground;
}

void displayUpdate(PlaydateAPI* pd) {
	PDButtons pushed;
	pd->system->getButtonState(&currentKeys, &pushed, NULL);
	// when the restart indicator is on, A sends ^D
	if (pythonInRepl && pythonWaitingForInput && (pushed & kButtonA)) {
		ringbuf_put(&stdin_ringbuf, 0x04);
	}
	collectedKeys |= currentKeys | pushed;

	if (dirty & eDirtyBackground) {
		pd->graphics->drawBitmap(background, 0, 0, kBitmapUnflipped);
		memset(frontbuf, 255, eBufferSize);
		dirty &= ~eDirtyBackground;
	}

	backbuf[eIndicatorMenu] = terminalUnread;
	backbuf[eIndicatorA] = (pythonInRepl && pythonWaitingForInput);

	uint8_t* frontpix = &frontbuf[0];
	uint8_t* backpix = &backbuf[0];
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			uint8_t c = *backpix;
			if (*frontpix != c) {
				*frontpix = c;
				LCDBitmap* bmp = pd->graphics->getTableBitmap(pixeltable, c);
				pd->graphics->drawBitmap(bmp, MX + TILEW * x, MY + TILEH * y, kBitmapUnflipped);
			}
			frontpix++;
			backpix++;
		}
	}
	for (struct Indicator* i = &indicators[0]; i->tableName != NULL; i++) {
		uint8_t c = *backpix;
		if (*frontpix != c) {
			*frontpix = c;
			LCDBitmap* bmp = pd->graphics->getTableBitmap(i->table, c);
			pd->graphics->drawBitmap(bmp, i->x, i->y, kBitmapUnflipped);
		}
		frontpix++;
		backpix++;
	}
}

mp_obj_t displayShow(mp_obj_t bufferobj, mp_obj_t width) {
	mp_buffer_info_t bi;
	mp_get_buffer_raise(bufferobj, &bi, MP_BUFFER_READ);
	mp_int_t w = mp_obj_get_int(width);
	if (w == 8 && (bi.typecode == BYTEARRAY_TYPECODE
		|| bi.typecode == 'B' || bi.typecode == 'b'))
	{
		if (bi.len > WIDTH*HEIGHT) {
			bi.len = WIDTH*HEIGHT;
		}
		for (int i = 0; i < bi.len; i++) {
			backbuf[i] = ((unsigned char*)bi.buf)[i] & 3;
		}
	}
	else {
		int n = bi.len / mp_binary_get_size('@', bi.typecode, NULL);
		int x = 0;
		int y = 0;
		for (size_t i = 0; i < n; i++) {
			if (x < WIDTH) {
				backbuf[y*WIDTH + x] = mp_obj_get_int(mp_binary_get_val_array(bi.typecode, bi.buf, i)) & 3;
			}
			if (++x == w) {
				x = 0;
				if (++y == HEIGHT) {
					break;
				}
			}
		}
	}
	return mp_const_none;
}

mp_obj_t displayKeys(void) {
	// getButtonState() also seems to return 64 for Menu, filter that out
	PDButtons k = collectedKeys & (kButtonLeft|kButtonRight|kButtonUp|kButtonDown|kButtonB|kButtonA);
	collectedKeys = currentKeys;
	return MP_OBJ_NEW_SMALL_INT(k);
}
