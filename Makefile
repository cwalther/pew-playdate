HEAP_SIZE      = 8388208
STACK_SIZE     = 61800

PRODUCT = pewpew.pdx
MICROPY_EMBED_DIR = micropython_embed

# Locate the SDK
SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
	SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif

ifeq ($(SDK),)
$(error SDK path not found; set ENV value PLAYDATE_SDK_PATH)
endif

######
# IMPORTANT: You must add your source folders to VPATH for make to find them
# ex: VPATH += src1:src2
######

VPATH += src

# List C source files here
SRC = src/main.c src/mphal.c src/terminal.c src/display.c src/preferences.c playdate-coroutines/pdco.c src/modules/_pew/mod_pew.c src/modules/_pew/vfs_pd.c src/modules/_pew/vfs_pd_file.c src/modules/c_hello/modc_hello.c
SRC += $(wildcard $(MICROPY_EMBED_DIR)/*/*.c)
# Filter out lib because the files in there cannot be compiled separately, they
# are #included by other .c files.
SRC += $(filter-out $(MICROPY_EMBED_DIR)/lib/%.c,$(wildcard $(MICROPY_EMBED_DIR)/*/*/*.c))

# List all user include directories here
UINCDIR = $(MICROPY_EMBED_DIR) $(MICROPY_EMBED_DIR)/port $(MICROPY_EMBED_DIR)/lib/littlefs

# List user asm files
UASRC =

# List all user C define here, like -D_DEBUG=1
UDEFS = -DNDEBUG

# Define ASM defines here
UADEFS =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk

# Include dummy implementations of syscalls to fix linker errors like "undefined
# reference to `_write'"; I have not figured out yet where in MicroPython these
# references come from and whether they could be avoided.
LDFLAGS += --specs=nosys.specs

micropython_embed/genhdr/qstrdefs.generated.h : micropython/ports/embed/port/micropython_embed.h mpconfigport.h
	$(MAKE) -f micropython_embed.mk
	touch micropython_embed/genhdr/qstrdefs.generated.h

micropython_embed/frozen/frozen_content.c : manifest.py $(wildcard src/modules/*.py)
	$(MAKE) -f micropython_embed.mk
	touch micropython_embed/frozen/frozen_content.c
