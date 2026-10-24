/*
 * wasm_api.c - Cau noi giua loi tinh toan C va JavaScript (ban demo web).
 *
 * Chi duoc bien dich khi build WebAssembly (make wasm). JavaScript ghi bieu
 * thuc vao input_buf, goi wasm_eval(), roi doc ket qua qua cac ham getter.
 */
#include "calc.h"

#define EXPORT(name) __attribute__((export_name(name)))

static CalcEnv    env;
static CalcResult last;
static char       input_buf[CALC_MAX_INPUT + 2];
static int        initialized = 0;

/* Khong co thu vien chuan: tu cung cap memset/memcpy phong khi trinh bien
 * dich sinh loi goi toi chung (khoi tao struct, sao chep struct). */
void *memset(void *dst, int c, unsigned long n) {
    volatile unsigned char *p = dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}

void *memcpy(void *dst, const void *src, unsigned long n) {
    volatile unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

EXPORT("input_buffer") char *wasm_input_buffer(void) { return input_buf; }
EXPORT("input_capacity") int wasm_input_capacity(void) { return CALC_MAX_INPUT + 1; }

EXPORT("reset") void wasm_reset(void) {
    calc_env_init(&env);
    initialized = 1;
}

EXPORT("eval") int wasm_eval(void) {
    if (!initialized) wasm_reset();
    last = calc_eval(&env, input_buf);
    return (int)last.error;
}

EXPORT("result") double wasm_result(void) { return last.value; }
EXPORT("error_pos") int wasm_error_pos(void) { return last.pos; }
EXPORT("assigned_name") const char *wasm_assigned_name(void) { return last.assigned; }
EXPORT("error_message") const char *wasm_error_message(int err) { return calc_error_message((CalcError)err); }

EXPORT("ans") double wasm_ans(void) { return env.ans; }
EXPORT("var_count") int wasm_var_count(void) { return env.var_count; }
EXPORT("var_name") const char *wasm_var_name(int i) {
    return (i >= 0 && i < env.var_count) ? env.vars[i].name : "";
}
EXPORT("var_value") double wasm_var_value(int i) {
    return (i >= 0 && i < env.var_count) ? env.vars[i].value : 0.0;
}
