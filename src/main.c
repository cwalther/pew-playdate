#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "port/micropython_embed.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "shared/runtime/pyexec.h"

#include "globals.h"
#include "terminal.h"

PlaydateAPI* global_pd;

static int update(void* userdata);
static void onSerialMessage(const char* data);

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

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	int stack_top;
	(void)arg; // arg is currently only used for event = kEventKeyPressed

	if (event == kEventInit) {
		// Note: If you set an update callback in the kEventInit handler, the system assumes the game is pure C and doesn't run any Lua code in the game
		pd->system->setUpdateCallback(&update, pd);

		pd->system->setSerialMessageCallback(&onSerialMessage);

		global_pd = pd;
		terminalInit(pd);

	#if MICROPY_STACK_CHECK
		mp_stack_set_limit(16384);
	#endif
		mp_embed_init(&heap[0], sizeof(heap), &stack_top);

		// Run the example scripts (they will be compiled first).
		mp_embed_exec_str(example_1);
		mp_embed_exec_str(example_2);

		pyexec_event_repl_init();

		// blocking REPL, try once we have coroutine switching
		//pyexec_friendly_repl();
	}
	else if (event == kEventTerminate) {
		mp_embed_deinit();
	}

	return 0;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

	terminalUpdate(pd);

	return 1;
}

static void onSerialMessage(const char* data) {
	while (*data != '\0') {
		pyexec_event_repl_process_char(*data++);
	}
	pyexec_event_repl_process_char('\r');
	pyexec_event_repl_process_char('\n');
}
