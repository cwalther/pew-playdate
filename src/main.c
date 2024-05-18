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

#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "playdate-coroutines/pdco.h"

//TODO remove this some time
#define EMBED_EXAMPLES 0
#if EMBED_EXAMPLES
#include "port/micropython_embed.h"
#endif
#include "py/gc.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"
#include "shared/runtime/interrupt_char.h"
#include "shared/readline/readline.h"

#include "globals.h"
#include "terminal.h"
#include "display.h"
#include "preferences.h"

#define PYTHON_STACK_SIZE 65536

PlaydateAPI* global_pd;
unsigned int updateEndDue;
int pythonInRepl;
int pythonWaitingForInput;

static int update(void* userdata);
static void onSerialMessage(const char* data);
static void onMenuInvert(void* userdata);
static void onMenuNavigate(void* userdata);
static pdco_handle_t pythonCoMain(pdco_handle_t caller);

typedef struct {
	void (*enter)(void);
	void (*update)(PlaydateAPI* pd);
} Card;

static Card displayCard = {
	displayTouch,
	displayUpdate
};
static Card terminalCard = {
	terminalTouch,
	terminalUpdate
};

#if EMBED_EXAMPLES
// This is example 1 script, which will be compiled and executed.
static const char *example_1 =
	"print('hello world!', list(x + 1.5 for x in range(10)), end='eol\\n')";

// This is example 2 script, which will be compiled and executed.
static const char *example_2 =
	"for i in range(10):\n"
	"	 print('iter {:08}'.format(i))\n"
	"\n"
	"try:\n"
	"	 1//0\n"
	"except Exception as er:\n"
	"	 print('caught exception', repr(er))\n"
	"\n"
	"import gc\n"
	"print('run GC collect')\n"
	"gc.collect()\n"
	"\n"
	"print('help(\\'modules\\'):')\n"
	"help('modules')\n"
	"import sys\n"
	"help(sys)\n"
	// Skip testing stdin when using the POSIX implementation, i.e. it is
	// connected to actual stdin, because that makes the program wait for input,
	// which can be inconvenient or confusing (giving the appearance of being
	// stuck). There is no harm enabling it if you remember to provide some
	// input.
#if !MICROPY_VFS_POSIX
	"print('sys.stdin.read(3):', repr(sys.stdin.read(3)))\n"
	"print('sys.stdin.buffer.read(3):', repr(sys.stdin.buffer.read(3)))\n"
#endif
	"sys.stdout.write('hello stdout text\\n')\n"
	"sys.stdout.buffer.write(b'hello stdout binary\\n')\n"
	"sys.stdout.write('hello stderr text\\n')\n"
	"sys.stdout.buffer.write(b'hello stderr binary\\n')\n"
	"import os\n"
	"help(os)\n"
	"print('os.uname():', os.uname())\n"
	"import random\n"
	"help(random)\n"
	"import time\n"
	"help(time)\n"
	"print('time.gmtime(736622952) = 2023-05-05T17:29:12Z:', time.gmtime(736622952))\n"
	"import math\n"
	"help(math)\n"
	"import uctypes\n"
	"help(uctypes)\n"
	"import frozenhello\n"
	"help(frozenhello)\n"
	"print('frozenhello.hello():', frozenhello.hello())\n"
	"import c_hello\n"
	"help(c_hello)\n"
	"print('c_hello.hello():', c_hello.hello())\n"
	"import fsimage, vfs\n"
	"vfs.mount(fsimage.BlockDev(), '/lfs', readonly=True)\n"
	"print('os.listdir(\\'/lfs\\'):', os.listdir('/lfs'))\n"
	"with open('/lfs/lfshello.py', 'rb') as f:\n"
	"	 print('lfshello.py:', f.read())\n"
	"sys.path.append('/lfs')\n"
	"import lfshello\n"
	"help(lfshello)\n"
	"print('lfshello.hello():', lfshello.hello())\n"
	"\n"
	"print('finish')\n"
	;
#endif

// This array is the MicroPython GC heap.
// TODO enlarge this some time, but while shaking out bugs it seems a good idea to hammer the GC
static char heap[16 * 1024];
static pdco_handle_t pythonCo;
static int pythonExit = 0;
static Card* currentCard;
static PDMenuItem* terminalItem;

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed

	if (event == kEventInit) {
		// Note: If you set an update callback in the kEventInit handler, the system assumes the game is pure C and doesn't run any Lua code in the game
		pd->system->setUpdateCallback(&update, pd);
		pd->system->setSerialMessageCallback(&onSerialMessage);
		pd->display->setRefreshRate(30.0f);

		global_pd = pd;
		preferencesRead(pd);
		terminalInit(pd);
		displayInit(pd);
		currentCard = &displayCard;

		PDMenuItem* item = pd->system->addCheckmarkMenuItem("invert", preferences.inverted, &onMenuInvert, NULL);
		pd->system->setMenuItemUserdata(item, item);
		terminalItem = pd->system->addMenuItem("terminal", &onMenuNavigate, NULL);

		pythonCo = pdco_create(&pythonCoMain, PYTHON_STACK_SIZE, NULL);
		if (pythonCo < 0) pd->system->logToConsole("pdco_create error");
	}
	else if (event == kEventPause) {
		pd->system->setMenuItemTitle(terminalItem, (currentCard == &terminalCard) ? "hide terminal" : terminalUnread ? "terminal \xE2\x9C\xA8" : "terminal");
		pd->system->setMenuItemUserdata(terminalItem, (currentCard == &terminalCard) ? &displayCard : &terminalCard);
	}
	else if (event == kEventTerminate) {
		pd->system->logToConsole("telling python to exit");
		pythonExit = 100;
		// this should get us back to the REPL, unless the user catches the exception
		mp_sched_keyboard_interrupt();
		// if we were already in the REPL with a non-empty input line, ^C clears it
		// ^D should get us out of the REPL
		stdin_ringbuf.iget = stdin_ringbuf.iput = 0; // clear the queue
		ringbuf_put(&stdin_ringbuf, 0x03);
		ringbuf_put(&stdin_ringbuf, 0x04);
		// if it didn't exit after 100 yields, screw it, quit without cleaning up
		while (pdco_exists(pythonCo) && --pythonExit > 0) {
			pdco_yield(pythonCo);
		}
		pd->system->logToConsole("python exited at %d", pythonExit);
	}

	return 0;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;
	// empirical: delays above 31 on the simulator and 33 on the device reduce
	// the frame rate to less than 30
	updateEndDue = pd->system->getCurrentTimeMilliseconds() + 32;

	currentCard->update(pd);

	pdco_yield(pythonCo);

	return 1;
}

static void onSerialMessage(const char* data) {
	if (data[0] == '!') {
		// base64: can encode any binary data
		data++;
		unsigned char acc = 0;
		unsigned char shift = 0;
		while (1) {
			unsigned char d = *data++;
			if (d == 0) {
				break;
			}
			else if (d == '+') {
				d = 62;
			}
			else if (d == '/') {
				d = 63;
			}
			else if (d >= '0' && d <= '9') {
				d = d - '0' + 52;
			}
			else if (d >= 'A' && d <= 'Z') {
				d = d - 'A' + 0;
			}
			else if (d >= 'a' && d <= 'z') {
				d = d - 'a' + 26;
			}
			else {
				continue;
			}

			if (shift != 0) {
				acc |= (d >> (6 - shift));
				if (acc == mp_interrupt_char) {
					mp_sched_keyboard_interrupt();
				}
				else {
					ringbuf_put(&stdin_ringbuf, acc);
				}
			}
			shift += 2;
			acc = (d << shift);
			shift &= 7;
		}
	}
	else {
		// literal data: convenient to enter manually
		ringbuf_put_bytes(&stdin_ringbuf, (const uint8_t*)data, strlen(data));
		ringbuf_put_bytes(&stdin_ringbuf, (const uint8_t*)"\r\n", 2);
	}
}

static void onMenuInvert(void* userdata) {
	int value = global_pd->system->getMenuItemValue((PDMenuItem *)userdata);
	displaySetInverted(global_pd, value);
	preferences.inverted = value;
	preferencesWrite(global_pd);
}

static void onMenuNavigate(void* userdata) {
	Card* newCard = userdata;
	if (currentCard != newCard) {
		if (currentCard == &terminalCard) {
			terminalUnread = 0;
		}
		currentCard = newCard;
		currentCard->enter();
	}
}

static pdco_handle_t pythonCoMain(pdco_handle_t caller) {
	int stack_top;
	mp_stack_set_top(&stack_top);
#if MICROPY_STACK_CHECK
	// deduction accounts for pdco STACKMARGIN and stuff above &stack_top
	mp_stack_set_limit(PYTHON_STACK_SIZE - 176);
#endif
	gc_init(&heap[0], &heap[0] + sizeof(heap));

	while (!pythonExit) {
		mp_init();
		mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));

		readline_init0();

		pyexec_frozen_module("_boot.py", false);
		int ret = pyexec_file_if_exists("boot.py");
		if (ret & PYEXEC_FORCED_EXIT) {
			goto soft_reset_exit;
		}
		if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL && ret != 0) {
			ret = pyexec_file_if_exists("main.py");
			if (ret & PYEXEC_FORCED_EXIT) {
				goto soft_reset_exit;
			}
		}

#if EMBED_EXAMPLES
		// Run the example scripts (they will be compiled first).
		mp_embed_exec_str(example_1);
		mp_embed_exec_str(example_2);
#endif

		pythonInRepl = 1;
		for (;;) {
			if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
				if (pyexec_raw_repl() != 0) {
					break;
				}
			} else {
				if (pyexec_friendly_repl() != 0) {
					break;
				}
			}
		}
		pythonInRepl = 0;

	soft_reset_exit:
		mp_printf(MP_PYTHON_PRINTER, "MPY: soft reboot\n");
		gc_sweep_all();
		mp_deinit();
	}

	global_pd->system->logToConsole("python exiting");
	return caller;
}
