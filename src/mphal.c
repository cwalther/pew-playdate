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
#if MICROPY_PY_OS_DUPTERM
#include "extmod/misc.h"
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
	return global_pd->system->getCurrentTimeMilliseconds();
}

#endif

#if MICROPY_PY_TIME

void mp_hal_delay_ms(mp_uint_t ms) {
	mp_int_t end = mp_hal_ticks_ms() + ms;
	mp_int_t remaining = ms;
	do {
		mp_event_wait_ms(remaining);
		remaining = end - mp_hal_ticks_ms();
	} while (remaining > 0);
}

void mp_hal_delay_us(mp_uint_t us) {
	mp_int_t end = mp_hal_ticks_us() + us;
	// If the delay is long (usually not), use the yielding function to work off
	// the bulk first. (Threshold 10000 is somewhat arbitrary.)
	// This is not required by the documentation and the bare-metal ports do not
	// do it, but seems useful on the Playdate where yielding is important for
	// responsive display/input and to avoid getting killed by the watchdog.
	if (us >= 10000) {
		mp_hal_delay_ms((us - 1000) / 1000);
	}
	// Then busy-wait for the rest.
	while (end - (mp_int_t)mp_hal_ticks_us() > 0) {}
}

mp_uint_t mp_hal_ticks_us(void) {
	return (mp_uint_t)(1.0e6f * global_pd->system->getElapsedTime());
}

// mp_hal_ticks_cpu #defined in mphalport.h

#endif

#if MICROPY_PY_TIME_TIME_TIME_NS

uint64_t mp_hal_time_ns(void) {
	// Nanoseconds since the Epoch.
	// Conveniently, Playdate has the same epoch as MicroPython (2000-01-01).
	unsigned int milliseconds;
	uint64_t seconds = global_pd->system->getSecondsSinceEpoch(&milliseconds);
	return 1000000000*seconds + 1000000*milliseconds;
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

#if MICROPY_PY_SYS_STDFILES && !MICROPY_VFS_POSIX

// Binary-mode standard input
// Receive single character, blocking until one is available.
int mp_hal_stdin_rx_chr(void) {
	pythonWaitingForInput = 1;
	for (;;) {
		int c = ringbuf_get(&stdin_ringbuf);
		if (c != -1) {
			pythonWaitingForInput = 0;
			return c;
		}
		#if MICROPY_PY_OS_DUPTERM
		int dupterm_c = mp_os_dupterm_rx_chr();
		if (dupterm_c >= 0) {
			pythonWaitingForInput = 0;
			return dupterm_c;
		}
		#endif
		mp_event_wait_indefinite();
	}
}

// Binary-mode standard output
// Send the string of given length.
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_uint_t ret = len;
    bool did_write = true;
	for (size_t i = 0; i < len; i++) {
		char c = str[i];
		pdSerialPutchar(c);
		terminalPutchar(c);
	}
	#if MICROPY_PY_OS_DUPTERM
	int dupterm_res = mp_os_dupterm_tx_strn(str, len);
	if (dupterm_res >= 0) {
		did_write = true;
		ret = MIN((mp_uint_t)dupterm_res, ret);
	}
	#endif
    return did_write ? ret : 0;
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
	#if MICROPY_PY_OS_DUPTERM
	ret |= mp_os_dupterm_poll(poll_flags);
	#endif
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
	if (end - (int)updateEndDue >= 0) {
		pdco_yield(PDCO_MAIN_ID);
	}
	else if (timeout_ms > 0) {
		while (end - global_pd->system->getCurrentTimeMilliseconds() > 0) {}
	}
}
