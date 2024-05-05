# Set the location of the top of the MicroPython repository.
MICROPYTHON_TOP = micropython

# Include modules from extmod in the output.
EMBED_EXTRA = extmod

# Include helper sources for the time module in the output.
EMBED_EXTRA += \
	shared/timeutils/timeutils.c \
	shared/timeutils/timeutils.h \
	shared/timeutils/modtime_mphal.h

# Include source for mphal-backed stdio in the output.
EMBED_EXTRA += \
	shared/runtime/sys_stdio_mphal.c \
	shared/runtime/stdout_helpers.c

# Include source for REPL and file execution in the output.
EMBED_EXTRA += \
	shared/runtime/pyexec.c \
	shared/runtime/pyexec.h \
	shared/readline/readline.c \
	shared/readline/readline.h \
	shared/runtime/interrupt_char.c \
	shared/runtime/interrupt_char.h

# Include library sources for littlefs 2 in the output.
EMBED_EXTRA += littlefs2

# Freeze Python modules.
FROZEN_MANIFEST ?= manifest.py

# Add C modules.
USER_C_MODULES = src/modules

# Include the main makefile fragment to build the MicroPython component.
include $(MICROPYTHON_TOP)/ports/embed/embed.mk
