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

#include <stdint.h>
#include <string.h>
#include <stdlib.h> //DEBUG

#define WIDTH 8
#define HEIGHT 8
#define TILEW 29
#define TILEH 29
#define MX (200 - (WIDTH/2)*TILEW)
#define MY (120 - (HEIGHT/2)*TILEH)

enum {
	eDirtyBackground = 1 << 0
};

static LCDBitmapTable* pixeltable;
static LCDBitmap* background;
static uint8_t frontbuf[WIDTH*HEIGHT];
static uint8_t backbuf[WIDTH*HEIGHT];
static uint8_t dirty;

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
	displaySetInverted(pd, preferences.inverted);
	memset(frontbuf, 255, WIDTH*HEIGHT);
}

void displayTouch(void) {
	dirty |= eDirtyBackground;
}

void displayUpdate(PlaydateAPI* pd) {
	backbuf[rand()%64] = rand()%4; //DEBUG

	if (dirty & eDirtyBackground) {
		pd->graphics->drawBitmap(background, 0, 0, kBitmapUnflipped);
		memset(frontbuf, 255, WIDTH*HEIGHT);
		dirty &= ~eDirtyBackground;
	}

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
}
