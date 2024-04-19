#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "playdate-coroutines/pdco.h"

#include "port/micropython_embed.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "globals.h"
#include "terminal.h"
#include "queue.h"

#define PYTHON_STACK_SIZE 65536

PlaydateAPI* global_pd;

static int update(void* userdata);
static void onSerialMessage(const char* data);
static pdco_handle_t pythonCoMain(pdco_handle_t caller);

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

// This array is the MicroPython GC heap.
// TODO enlarge this some time, but while shaking out bugs it seems a good idea to hammer the GC
static char heap[16 * 1024];
static pdco_handle_t pythonCo;
static int pythonExit = 0;
static Queue stdinQueue;

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

		global_pd = pd;
		terminalInit(pd);
		queueInit(&stdinQueue);

		pythonCo = pdco_create(&pythonCoMain, PYTHON_STACK_SIZE, NULL);
		if (pythonCo < 0) pd->system->logToConsole("pdco_create error");
	}
	else if (event == kEventTerminate) {
		pd->system->logToConsole("telling python to exit");
		pythonExit = 1;
		pdco_yield(pythonCo);
		pd->system->logToConsole("python exited");
	}

	return 0;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

	terminalUpdate(pd);

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
				queueWrite(&stdinQueue, &acc, 1);
			}
			shift += 2;
			acc = (d << shift);
			shift &= 7;
		}
	}
	else {
		// literal data: convenient to enter manually
		queueWrite(&stdinQueue, data, strlen(data));
		queueWrite(&stdinQueue, "\r\n", 2);
	}
}

static pdco_handle_t pythonCoMain(pdco_handle_t caller) {
	int stack_top;
#if MICROPY_STACK_CHECK
	// deduction accounts for pdco STACKMARGIN and stuff above &stack_top
	mp_stack_set_limit(PYTHON_STACK_SIZE - 176);
#endif
	mp_embed_init(&heap[0], sizeof(heap), &stack_top);

	// Run the example scripts (they will be compiled first).
	mp_embed_exec_str(example_1);
	mp_embed_exec_str(example_2);

	pyexec_event_repl_init();

	// blocking REPL, try once we have coroutine switching
	//pyexec_friendly_repl();

	while (!pythonExit) {
		char c;
		while (queueRead(&stdinQueue, &c, 1) > 0) {
			pyexec_event_repl_process_char(c);
		}
		pdco_yield(caller);
	}
	global_pd->system->logToConsole("python exiting");

	mp_embed_deinit();

	return caller;
}
