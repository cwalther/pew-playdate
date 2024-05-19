/*
Copyright (c) 2024 Christian Walther

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

#include "py/runtime.h"
#include "py/objstr.h"

#include "vfs_pd.h"

#if defined(TARGET_PLAYDATE) || defined(TARGET_SIMULATOR)
#include "src/display.h"
// #else we are in the micropython_embed build which only preprocesses but does
// not compile, and does not have the Playdate SDK
#endif

static MP_DEFINE_CONST_FUN_OBJ_2(show_obj, displayShow);

static const mp_rom_map_elem_t pew_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__pew) },
	{ MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&show_obj) },
    { MP_ROM_QSTR(MP_QSTR_VfsPD), MP_ROM_PTR(&mp_type_vfs_pd) },
};
static MP_DEFINE_CONST_DICT(pew_module_globals, pew_module_globals_table);

const mp_obj_module_t pew_module = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&pew_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__pew, pew_module);
