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
#include "py/stream.h"

#include "vfs_pd.h"
#if defined(TARGET_PLAYDATE) || defined(TARGET_SIMULATOR)
#include "src/globals.h"
// #else we are in the micropython_embed build which only preprocesses but does
// not compile, and does not have the Playdate SDK
#endif

typedef struct _mp_obj_vfs_pd_file_t {
	mp_obj_base_t base;
	SDFile* sdfile;
} mp_obj_vfs_pd_file_t;

static void vfs_pd_file_check_open(mp_obj_vfs_pd_file_t * self) {
	if (self->sdfile == NULL) {
		mp_raise_ValueError(MP_ERROR_TEXT("I/O operation on closed file"));
	}
}

static void vfs_pd_file_print(const mp_print_t * print, mp_obj_t self_in, mp_print_kind_t kind) {
	(void)self_in;
	(void)kind;
	mp_printf(print, "<io.%s>", mp_obj_get_type_str(self_in));
}

static mp_uint_t vfs_pd_file_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
	mp_obj_vfs_pd_file_t *self = MP_OBJ_TO_PTR(self_in);
	vfs_pd_file_check_open(self);
	int len = global_pd->file->read(self->sdfile, buf, size);
	if (len < 0) {
		*errcode = MP_EIO;
		return MP_STREAM_ERROR;
	}
	return len;
}

static mp_uint_t vfs_pd_file_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
	mp_obj_vfs_pd_file_t *self = MP_OBJ_TO_PTR(self_in);
	vfs_pd_file_check_open(self);
	int len = global_pd->file->write(self->sdfile, buf, size);
	if (len < 0) {
		*errcode = MP_EIO;
		return MP_STREAM_ERROR;
	}
	return len;
}

static mp_uint_t vfs_pd_file_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
	mp_obj_vfs_pd_file_t *self = MP_OBJ_TO_PTR(self_in);

	if (request != MP_STREAM_CLOSE) {
		vfs_pd_file_check_open(self);
	}

	if (request == MP_STREAM_SEEK) {
		struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)(uintptr_t)arg;
		int res = global_pd->file->seek(self->sdfile, s->offset, s->whence);
		if (res < 0) {
			*errcode = MP_EINVAL;
			return MP_STREAM_ERROR;
		}
		res = global_pd->file->tell(self->sdfile);
		if (res < 0) {
			*errcode = MP_EIO;
			return MP_STREAM_ERROR;
		}
		s->offset = res;
		return 0;
	}
	else if (request == MP_STREAM_FLUSH) {
		int res = global_pd->file->flush(self->sdfile);
		if (res < 0) {
			*errcode = MP_EIO;
			return MP_STREAM_ERROR;
		}
		return 0;
	}
	else if (request == MP_STREAM_CLOSE) {
		if (self->sdfile == NULL) {
			return 0;
		}
		int res = global_pd->file->close(self->sdfile);
		self->sdfile = NULL; // indicate a closed file
		if (res < 0) {
			*errcode = MP_EIO;
			return MP_STREAM_ERROR;
		}
		return 0;
	}
	else {
		*errcode = MP_EINVAL;
		return MP_STREAM_ERROR;
	}
}

static const mp_stream_p_t vfs_pd_fileio_stream_p = {
	.read = vfs_pd_file_read,
	.write = vfs_pd_file_write,
	.ioctl = vfs_pd_file_ioctl,
};

static const mp_stream_p_t vfs_pd_textio_stream_p = {
	.read = vfs_pd_file_read,
	.write = vfs_pd_file_write,
	.ioctl = vfs_pd_file_ioctl,
	.is_text = true,
};

static const mp_rom_map_elem_t vfs_pd_rawfile_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
	{ MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
	{ MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
	{ MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
	{ MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
	{ MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
	{ MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
	{ MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
	{ MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};
static MP_DEFINE_CONST_DICT(vfs_pd_rawfile_locals_dict, vfs_pd_rawfile_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
	mp_type_vfs_pd_fileio,
	MP_QSTR_FileIO,
	MP_TYPE_FLAG_ITER_IS_STREAM,
	print, vfs_pd_file_print,
	protocol, &vfs_pd_fileio_stream_p,
	locals_dict, &vfs_pd_rawfile_locals_dict
	);

MP_DEFINE_CONST_OBJ_TYPE(
	mp_type_vfs_pd_textio,
	MP_QSTR_TextIOWrapper,
	MP_TYPE_FLAG_ITER_IS_STREAM,
	print, vfs_pd_file_print,
	protocol, &vfs_pd_textio_stream_p,
	locals_dict, &vfs_pd_rawfile_locals_dict
	);

mp_obj_t vfs_pd_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in) {
	mp_obj_vfs_pd_t *self = MP_OBJ_TO_PTR(self_in);
	FileOptions mode = kFileRead|kFileReadData;
	const mp_obj_type_t *type = &mp_type_vfs_pd_textio;
	const char *mode_str = mp_obj_str_get_str(mode_in);
	for (; *mode_str; ++mode_str) {
		switch (*mode_str) {
			case 'r':
				mode = kFileRead|kFileReadData;
				break;
			case 'w':
				mode = kFileWrite;
				break;
			case 'a':
				mode = kFileAppend;
				break;
			case '+':
				// w+b does not truncate as specified by CPython, but otherwise this works
				if (mode & (kFileRead|kFileReadData)) mode |= kFileAppend;
				else mode |= kFileRead|kFileReadData;
				break;
			case 'b':
				type = &mp_type_vfs_pd_fileio;
				break;
			case 't':
				type = &mp_type_vfs_pd_textio;
				break;
		}
	}
	if (self->readonly && (mode & (kFileWrite|kFileAppend))) {
		mp_raise_OSError(MP_EROFS);
	}
	const char *path = vfs_pd_make_path(self, path_in);
	SDFile* f = global_pd->file->open(path, mode);
	if (f == NULL) {
		raise_OSError_pd();
	}
	mp_obj_vfs_pd_file_t *o = mp_obj_malloc_with_finaliser(mp_obj_vfs_pd_file_t, type);
	o->sdfile = f;
	return MP_OBJ_FROM_PTR(o);
}
