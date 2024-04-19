#include "py/runtime.h"
#include "py/objstr.h"

#include "playdate-coroutines/pdco.h"

static mp_obj_t pew_yield_to_pd(void) {
	pdco_yield(PDCO_MAIN_ID);
	return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(pew_yield_to_pd_obj, pew_yield_to_pd);

static const mp_rom_map_elem_t pew_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__pew) },
	{ MP_ROM_QSTR(MP_QSTR_yield_to_pd), MP_ROM_PTR(&pew_yield_to_pd_obj) },
};
static MP_DEFINE_CONST_DICT(pew_module_globals, pew_module_globals_table);

const mp_obj_module_t pew_module = {
	.base = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&pew_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__pew, pew_module);
