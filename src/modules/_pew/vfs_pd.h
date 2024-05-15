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

#pragma once

#include "py/obj.h"

extern const mp_obj_type_t mp_type_vfs_pd;

typedef struct _mp_obj_vfs_pd_t {
	mp_obj_base_t base;
	// ends but does not start with '/' (as many '/' as components)
	vstr_t root;
	// relative to root, starts and ends with '/' (one more '/' than components)
	vstr_t cur_dir;
	size_t root_len;
	bool readonly;
} mp_obj_vfs_pd_t;

const char *vfs_pd_make_path(mp_obj_vfs_pd_t *self, mp_obj_t path_in);
NORETURN void raise_OSError_pd(void);
mp_obj_t vfs_pd_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in);
