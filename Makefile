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
SRC = src/main.c src/mphal.c src/terminal.c src/modules/c_hello/modc_hello.c
SRC += $(wildcard $(MICROPY_EMBED_DIR)/*/*.c)
# Filter out lib because the files in there cannot be compiled separately, they
# are #included by other .c files.
SRC += $(filter-out $(MICROPY_EMBED_DIR)/lib/%.c,$(wildcard $(MICROPY_EMBED_DIR)/*/*/*.c))

# List all user include directories here
UINCDIR = $(MICROPY_EMBED_DIR) $(MICROPY_EMBED_DIR)/port $(MICROPY_EMBED_DIR)/lib/littlefs

# List user asm files
UASRC =

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk

micropython_embed/port/micropython_embed.h: micropython/ports/embed/port/micropython_embed.h mpconfigport.h manifest.py
	$(MAKE) -f micropython_embed.mk

build/src/main.o: micropython_embed/port/micropython_embed.h
