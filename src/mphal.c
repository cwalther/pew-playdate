/*
Copyright (c) 2023-2024 Christian Walther

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

#include "py/builtin.h"
#include "py/compile.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#if MICROPY_PY_SYS_STDFILES
#include "py/stream.h"
#endif

#include "playdate-coroutines/pdco.h"

#include "globals.h"
#include "terminal.h"

#ifndef MICROPY_HW_STDIN_BUFFER_LEN
#define MICROPY_HW_STDIN_BUFFER_LEN 512
#endif
static uint8_t stdin_ringbuf_array[MICROPY_HW_STDIN_BUFFER_LEN];
ringbuf_t stdin_ringbuf = { stdin_ringbuf_array, sizeof(stdin_ringbuf_array) };

#if !MICROPY_VFS

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
	mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
	(void)path;
	return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

#endif

#if MICROPY_PY_ASYNCIO

mp_uint_t mp_hal_ticks_ms(void) {
	return 0;
}

#endif

#if MICROPY_PY_TIME

void mp_hal_delay_ms(mp_uint_t ms) {
}

void mp_hal_delay_us(mp_uint_t us) {
}

mp_uint_t mp_hal_ticks_us(void) {
	return 0;
}

mp_uint_t mp_hal_ticks_cpu(void) {
	return 0;
}

#endif

#if MICROPY_PY_TIME_TIME_TIME_NS

uint64_t mp_hal_time_ns(void) {
	// Nanoseconds since the Epoch.
	return 0;
}

#endif

// Can't output without a trailing '\n', so line-buffer for now
// (https://devforum.play.date/t/logtoconsole-without-a-linebreak/1819/6)
static void pdSerialPutchar(char c) {
	static char buffer[256];
	static size_t cursor = 0;
	if (c == '\n') {
		global_pd->system->logToConsole("%.*s", (int)cursor, buffer);
		cursor = 0;
	}
	else {
		if (cursor == sizeof(buffer)/sizeof(buffer[0])) {
			global_pd->system->logToConsole("%.*s", (int)cursor, buffer);
			cursor = 0;
		}
		buffer[cursor++] = c;
	}
}

// Text-mode standard output
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
	for (size_t i = 0; i < len; i++) {
		char c = str[i];
		pdSerialPutchar(c);
		if (c == '\n') {
			terminalPutchar('\r');
		}
		terminalPutchar(c);
	}
}

#if MICROPY_PY_SYS_STDFILES && !MICROPY_VFS_POSIX

// Binary-mode standard input
// Receive single character, blocking until one is available.
int mp_hal_stdin_rx_chr(void) {
	for (;;) {
		int c = ringbuf_get(&stdin_ringbuf);
		if (c != -1) {
			return c;
		}
		//TODO support MICROPY_PY_OS_DUPTERM
		mp_event_wait_indefinite();
	}
}

// Binary-mode standard output
// Send the string of given length.
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
	for (size_t i = 0; i < len; i++) {
		char c = str[i];
		pdSerialPutchar(c);
		terminalPutchar(c);
	}
	return len;
}

void mp_hal_stdout_tx_str(const char *str) {
	mp_hal_stdout_tx_strn(str, strlen(str));
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
	uintptr_t ret = 0;
	if ((poll_flags & MP_STREAM_POLL_RD) && ringbuf_peek(&stdin_ringbuf) != -1) {
		ret |= MP_STREAM_POLL_RD;
	}
	if (poll_flags & MP_STREAM_POLL_WR) {
		// can always write
		ret |= MP_STREAM_POLL_WR;
	}
	return ret;
}

#endif

void pd_hal_wfe_indefinite(void) {
	// Wait for event indefinitely, called from mp_event_wait_indefinite():
	// Just yield and wait for the next update().
	pdco_yield(PDCO_MAIN_ID);
}

void pd_hal_wfe_ms(int timeout_ms) {
	// Wait for event with timeout, called from mp_event_wait_ms() and from
	// MICROPY_VM_HOOK_POLL: If update() is due to return within the timeout
	// (or already in the past), yield and wait for the next call. If not,
	// yielding would wait for too long, so just busy-wait (there does not seem
	// to be a sleep function in the Playdate API) or return immediately for
	// timeout 0 as from MICROPY_VM_HOOK_POLL.
	int end = global_pd->system->getCurrentTimeMilliseconds() + timeout_ms;
	if (end - updateEndDue >= 0) {
		pdco_yield(PDCO_MAIN_ID);
	}
	else if (timeout_ms > 0) {
		while (end - global_pd->system->getCurrentTimeMilliseconds() > 0) {}
	}
}
