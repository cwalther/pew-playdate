// Include common MicroPython embed configuration.
#include <port/mpconfigport_common.h>

// apparently there is no SSIZE_MAX in the Playdate SDK
#define MP_SSIZE_MAX (SIZE_MAX/2)

// Use the same starting configuration as on most bare-metal targets.
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)

// MicroPython configuration.
#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                           (1)

#define MICROPY_PY_SYS_PLATFORM                 "Playdate"
#define MICROPY_PY_OS_UNAME                     (1)
#define MICROPY_HW_BOARD_NAME                   "PewPew PD"
#define MICROPY_HW_MCU_NAME                     "STM32"

// Enable the VFS subsystem, littlefs, importing from VFS, and the vfs module.
// TODO we probably won't need littlefs, but leave it on until we have a
// Playdate FS.
#define MICROPY_VFS                             (1)
#define MICROPY_VFS_LFS2                        (1)
#define MICROPY_READER_VFS                      (1)
#define MICROPY_PY_VFS                          (1)

// Enable floating point numbers and the math module.
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)

// Enable more functions in the time module. Requires additions to EMBED_EXTRA,
// see micropython_embed.mk.
#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME (1)
#define MICROPY_PY_TIME_TIME_TIME_NS            (1)
#define MICROPY_PY_TIME_INCLUDEFILE             "shared/timeutils/modtime_mphal.h"

// Requires shared/readline/readline.h, don't bother as we have no input.
// TODO actually we do, try this some time
#define MICROPY_PY_BUILTINS_INPUT               (0)

// Requires MICROPY_EVENT_POLL_HOOK, don't bother as we have no pollable objects.
#define MICROPY_PY_SELECT                       (0)

// We have our own implementation of mp_hal_stdout_tx_strn_cooked().
#undef MP_PLAT_PRINT_STRN

// Enable freezing of Python modules. These would be set automatically for the
// port build based on the presence of FROZEN_MANIFEST by py/mkrules.mk, but
// must be set explicitly for the application build.
#define MICROPY_MODULE_FROZEN_MPY               1
#define MICROPY_MODULE_FROZEN_STR               1
#define MICROPY_QSTR_EXTRA_POOL                 mp_qstr_frozen_const_pool
// must match MPY_TOOL_FLAGS or defaults for mpy-tool.py arguments
#define MICROPY_LONGINT_IMPL                    (MICROPY_LONGINT_IMPL_MPZ)
#define MPZ_DIG_SIZE                            (16)

#define MP_STATE_PORT MP_STATE_VM
