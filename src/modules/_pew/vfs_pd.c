/*
Copyright (c) 2024 Christian Walther, 2017-2020 Damien P. George

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
#include "py/mperrno.h"
#include "extmod/vfs.h"
#include "shared/timeutils/timeutils.h"

#include "vfs_pd.h"
#if defined(TARGET_PLAYDATE) || defined(TARGET_SIMULATOR)
#include "src/globals.h"
// #else we are in the micropython_embed build which only preprocesses but does
// not compile, and does not have the Playdate SDK
#endif

// input: path relative to root if starting with '/', else relative to cur_dir
// output: full path ready for Playdate APIs, no leading or trailing /, no . or ..
const char *vfs_pd_make_path(mp_obj_vfs_pd_t *self, mp_obj_t path_in) {
	const char *path = mp_obj_str_get_str(path_in);
	self->root.len = self->root_len;
	// the previous call may have eaten up the trailing / if it referred to the
	// root, put it back
	if (self->root_len > 0) {
		self->root.buf[self->root_len-1] = '/';
	}
	if (path[0] == '/') {
		path += 1;
	}
	else {
		vstr_add_strn(&self->root, vstr_str(&self->cur_dir)+1, vstr_len(&self->cur_dir)-1);
	}
	vstr_add_str(&self->root, path);

	// normalize
	// add a slash so that every component is slash-terminated
	vstr_add_byte(&self->root, '/');
	#define PATH_LEN (vstr_len(&self->root))
	size_t to = 0;
	size_t from = 0;
	char *cwd = vstr_str(&self->root);
	while (from < PATH_LEN) {
		for (; from < PATH_LEN && cwd[from] == '/'; ++from) {
			// Scan for the start
		}
		if (from > to) {
			// Found excessive slash chars, squeeze them out
			vstr_cut_out_bytes(&self->root, to, from - to);
			from = to;
		}
		for (; from < PATH_LEN && cwd[from] != '/'; ++from) {
			// Scan for the next /
		}
		if ((from - to) == 1 && cwd[to] == '.') {
			// './', ignore
			vstr_cut_out_bytes(&self->root, to, ++from - to);
			from = to;
		} else if ((from - to) == 2 && cwd[to] == '.' && cwd[to + 1] == '.') {
			// '../', skip back
			if (to > self->root_len) {
				// Only skip back if not at the tip
				for (--to; to > self->root_len && cwd[to - 1] != '/'; --to) {
					// Skip back
				}
			}
			vstr_cut_out_bytes(&self->root, to, ++from - to);
			from = to;
		} else {
			// Normal element, keep it and just move the offset
			to = ++from;
		}
	}
	// if there are more than 0 components, remove the terminating / of the last
	vstr_cut_tail_bytes(&self->root, 1);

	return vstr_null_terminated_str(&self->root);
}

NORETURN void raise_OSError_pd(void) {
	const char* msg = global_pd->file->geterr();
	mp_obj_t o_str = mp_obj_new_str_from_cstr(msg);
	// ENOENT isn't always the correct error code, but often, and there's no way
	// to find out (except, fragile, checking for known strings).
	mp_obj_t args[2] = { MP_OBJ_NEW_SMALL_INT(MP_ENOENT), MP_OBJ_FROM_PTR(o_str)};
	nlr_raise(mp_obj_exception_make_new(&mp_type_OSError, 2, 0, args));
}

static mp_import_stat_t mp_vfs_pd_import_stat(void *self_in, const char *path) {
	mp_obj_vfs_pd_t *self = self_in;
	if (self->root_len != 0) {
		self->root.len = self->root_len;
		if (self->root_len > 0) {
			self->root.buf[self->root_len-1] = '/';
		}
		vstr_add_str(&self->root, path);
		path = vstr_null_terminated_str(&self->root);
	}
	FileStat st;
	if (global_pd->file->stat(path, &st) == 0) {
		return st.isdir ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
	}
	return MP_IMPORT_STAT_NO_EXIST;
}

static mp_obj_t vfs_pd_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
	mp_arg_check_num(n_args, n_kw, 0, 1, false);

	mp_obj_vfs_pd_t *vfs = mp_obj_malloc(mp_obj_vfs_pd_t, type);
	vstr_init(&vfs->root, 0);
	if (n_args == 1) {
		const char *root = mp_obj_str_get_str(args[0]);
		while (root[0] == '/') {
			root++;
		}
		vstr_add_str(&vfs->root, root);
		if (vfs->root.len > 0) {
			vstr_add_byte(&vfs->root, '/');
		}
	}
	vfs->root_len = vfs->root.len;
	vstr_init(&vfs->cur_dir, 1);
	vstr_add_byte(&vfs->cur_dir, '/');
	vfs->readonly = false;

	return MP_OBJ_FROM_PTR(vfs);
}

static mp_obj_t vfs_pd_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	if (mp_obj_is_true(readonly)) {
		self->readonly = true;
	}
	if (mp_obj_is_true(mkfs)) {
		mp_raise_OSError(MP_EPERM);
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_pd_mount_obj, vfs_pd_mount);

static mp_obj_t vfs_pd_umount(mp_obj_t self_in) {
	(void)self_in;
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(vfs_pd_umount_obj, vfs_pd_umount);

// vfs_pd_open implemented in vfs_pd_file.c
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_pd_open_obj, vfs_pd_open);

static mp_obj_t vfs_pd_chdir(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);

	// Check path exists
	const char *path = vfs_pd_make_path(self, path_in);
	if (path[0] != '\0') {
		// Not at root, check it exists
		FileStat st;
		if (global_pd->file->stat(path, &st) < 0 || !st.isdir) {
			mp_raise_OSError(MP_ENOENT);
		}
	}

	// Update cur_dir with new path
	vstr_reset(&self->cur_dir);
	vstr_add_byte(&self->cur_dir, '/');
	vstr_add_str(&self->cur_dir, path + self->root_len);
	vstr_add_byte(&self->cur_dir, '/');

	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_pd_chdir_obj, vfs_pd_chdir);

static mp_obj_t vfs_pd_getcwd(mp_obj_t self_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	if (vstr_len(&self->cur_dir) == 1) {
		return MP_OBJ_NEW_QSTR(MP_QSTR__slash_);
	} else {
		// include leading but not trailing /
		return mp_obj_new_str(self->cur_dir.buf, self->cur_dir.len - 1);
	}
}
static MP_DEFINE_CONST_FUN_OBJ_1(vfs_pd_getcwd_obj, vfs_pd_getcwd);

typedef struct {
	mp_obj_t list;
	bool is_str;
} vfs_pd_ilistdir_userdata_t;

static void vfs_pd_ilistdir_callback(const char* filename, void* userdata) {
	mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));
	int isdir = (strchr(filename, '/') != NULL);
	if (((vfs_pd_ilistdir_userdata_t*)userdata)->is_str) {
		t->items[0] = mp_obj_new_str(filename, strlen(filename) - isdir);
	}
	else {
		t->items[0] = mp_obj_new_bytes((const byte *)filename, strlen(filename) - isdir);
	}
	t->items[1] = MP_OBJ_NEW_SMALL_INT(isdir ? MP_S_IFDIR : MP_S_IFREG);
	t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // no inode number
	mp_obj_list_append(((vfs_pd_ilistdir_userdata_t*)userdata)->list, MP_OBJ_FROM_PTR(t));
}

static mp_obj_t vfs_pd_ilistdir(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	vfs_pd_ilistdir_userdata_t ud;
	ud.list = mp_obj_new_list(0, NULL);
	ud.is_str = (mp_obj_get_type(path_in) == &mp_type_str);
	if (global_pd->file->listfiles(vfs_pd_make_path(self, path_in), &vfs_pd_ilistdir_callback, &ud, true) < 0) {
		raise_OSError_pd();
	}
	return mp_getiter(ud.list, NULL);
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_pd_ilistdir_obj, vfs_pd_ilistdir);

static mp_obj_t vfs_pd_mkdir(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	if (self->readonly) {
		mp_raise_OSError(MP_EROFS);
	}
	const char *path = vfs_pd_make_path(self, path_in);
	if (global_pd->file->mkdir(path) < 0) {
		raise_OSError_pd();
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_pd_mkdir_obj, vfs_pd_mkdir);

static mp_obj_t vfs_pd_remove(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	if (self->readonly) {
		mp_raise_OSError(MP_EROFS);
	}
	const char *path = vfs_pd_make_path(self, path_in);
	if (global_pd->file->unlink(path, false) < 0) {
		raise_OSError_pd();
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_pd_remove_obj, vfs_pd_remove);

static mp_obj_t vfs_pd_rename(mp_obj_t self_in, mp_obj_t old_path_in, mp_obj_t new_path_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	if (self->readonly) {
		mp_raise_OSError(MP_EROFS);
	}
	// must copy the first path because the second call to vfs_pd_make_path overwrites it
	const char *old_path = vfs_pd_make_path(self, old_path_in);
	size_t len = strlen(old_path);
	vstr_t old_path_v;
	vstr_init(&old_path_v, len);
	vstr_add_strn(&old_path_v, old_path, len);
	old_path = vstr_null_terminated_str(&old_path_v);
	const char *new_path = vfs_pd_make_path(self, new_path_in);
	int res = global_pd->file->rename(old_path, new_path);
	vstr_clear(&old_path_v);
	if (res < 0) {
		raise_OSError_pd();
	}
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_pd_rename_obj, vfs_pd_rename);

static mp_obj_t vfs_pd_stat(mp_obj_t self_in, mp_obj_t path_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	const char *path = vfs_pd_make_path(self, path_in);
	FileStat st;
	if (global_pd->file->stat(path, &st) < 0) {
		raise_OSError_pd();
	}
	mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
	t->items[0] = MP_OBJ_NEW_SMALL_INT((st.isdir ? MP_S_IFDIR : MP_S_IFREG)); // mode
	t->items[1] = MP_OBJ_NEW_SMALL_INT(0); // ino
	t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // dev
	t->items[3] = MP_OBJ_NEW_SMALL_INT(0); // nlink
	t->items[4] = MP_OBJ_NEW_SMALL_INT(0); // uid
	t->items[5] = MP_OBJ_NEW_SMALL_INT(0); // gid
	t->items[6] = mp_obj_new_int_from_uint(st.size); // size
	mp_uint_t time = timeutils_seconds_since_2000(st.m_year, st.m_month, st.m_day, st.m_hour, st.m_minute, st.m_second);
	t->items[7] = mp_obj_new_int_from_uint(time); // atime
	t->items[8] = mp_obj_new_int_from_uint(time); // mtime
	t->items[9] = mp_obj_new_int_from_uint(time); // ctime
	return MP_OBJ_FROM_PTR(t);
}
static MP_DEFINE_CONST_FUN_OBJ_2(vfs_pd_stat_obj, vfs_pd_stat);

static const mp_rom_map_elem_t vfs_pd_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&vfs_pd_mount_obj) },
	{ MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&vfs_pd_umount_obj) },
	{ MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&vfs_pd_open_obj) },

	{ MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&vfs_pd_chdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&vfs_pd_getcwd_obj) },
	{ MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&vfs_pd_ilistdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&vfs_pd_mkdir_obj) },
	{ MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&vfs_pd_remove_obj) },
	{ MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&vfs_pd_rename_obj) },
	{ MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&vfs_pd_remove_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&vfs_pd_stat_obj) },
	// there's no Playdate API for statvfs
};
static MP_DEFINE_CONST_DICT(vfs_pd_locals_dict, vfs_pd_locals_dict_table);

static const mp_vfs_proto_t vfs_pd_proto = {
	.import_stat = mp_vfs_pd_import_stat,
};

MP_DEFINE_CONST_OBJ_TYPE(
	mp_type_vfs_pd,
	MP_QSTR_VfsPD,
	MP_TYPE_FLAG_NONE,
	make_new, vfs_pd_make_new,
	protocol, &vfs_pd_proto,
	locals_dict, &vfs_pd_locals_dict
	);
