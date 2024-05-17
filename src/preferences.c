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

#include "preferences.h"

#include <string.h>

Preferences preferences;

typedef struct {
	PlaydateAPI* pd;
	SDFile* file;
} PrefsUserdata;

static void prefsDecodeError(json_decoder* decoder, const char* error, int linenum) {
	((PrefsUserdata*)(decoder->userdata))->pd->system->error("JSON decode error on line %d: %s", linenum, error);
}

static int prefsShouldDecodeTableValueForKey(json_decoder* decoder, const char* key) {
	// skip anything we're not interested in
	return strcmp(decoder->path, "_root") == 0
		&& strcmp(key, "inverted") == 0;
}

static void prefsDidDecodeTableValue(json_decoder* decoder, const char* key, json_value value) {
	// we must be at _root because prefsShouldDecodeTableValueForKey skipped everything else
	if ((value.type == kJSONTrue || value.type == kJSONFalse) && strcmp(key, "inverted") == 0) {
		preferences.inverted = json_boolValue(value);
	}
}

static int readfile(void* userdata, uint8_t* buf, int bufsize) {
	return ((PrefsUserdata*)userdata)->pd->file->read(((PrefsUserdata*)userdata)->file, buf, bufsize);
}

void preferencesRead(PlaydateAPI* pd) {
	PrefsUserdata ud;
	ud.pd = pd;
	json_decoder decoder = {
		.decodeError = prefsDecodeError,
		.shouldDecodeTableValueForKey = prefsShouldDecodeTableValueForKey,
		.didDecodeTableValue = prefsDidDecodeTableValue,
		.userdata = &ud
	};
	preferences.inverted = 0;
	ud.file = pd->file->open("data.json", kFileReadData);
	if (ud.file != NULL) {
		pd->json->decode(&decoder, (json_reader){ .read = readfile, .userdata = &ud }, NULL);
		pd->file->close(ud.file);
	}
	// else probably file doesn't exist yet, that's OK
}

static void writefile(void* userdata, const char* str, int len) {
	((PrefsUserdata*)userdata)->pd->file->write(((PrefsUserdata*)userdata)->file, str, len);
}

void preferencesWrite(PlaydateAPI* pd) {
	PrefsUserdata ud;
	ud.pd = pd;
	json_encoder encoder;
	ud.file = pd->file->open("data.json", kFileWrite);
	if (ud.file != NULL) {
		pd->json->initEncoder(&encoder, &writefile, &ud, 0);
		encoder.startTable(&encoder);
		encoder.addTableMember(&encoder, "inverted", sizeof("inverted")-1);
		if (preferences.inverted) {
			encoder.writeTrue(&encoder);
		}
		else {
			encoder.writeFalse(&encoder);
		}
		encoder.endTable(&encoder);
		pd->file->close(ud.file);
	}
}
