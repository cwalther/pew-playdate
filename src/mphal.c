#include "py/builtin.h"
#include "py/compile.h"
#include "py/mperrno.h"
#if MICROPY_PY_SYS_STDFILES
#include "py/stream.h"
#endif

#include "globals.h"
#include "terminal.h"

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
static const char* input = "hello\nworld\n";
int mp_hal_stdin_rx_chr(void) {
	if (*input == '\0') return 0;
	return *input++;
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
	if ((poll_flags & MP_STREAM_POLL_RD) && *input != '\0') {
		ret |= MP_STREAM_POLL_RD;
	}
	if (poll_flags & MP_STREAM_POLL_WR) {
		// can always write
		ret |= MP_STREAM_POLL_WR;
	}
	return ret;
}

#endif
