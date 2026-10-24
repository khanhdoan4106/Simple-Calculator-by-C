/*
 * calc.h - Bo phan tich va tinh gia tri bieu thuc.
 *
 * Quy trinh:  chuoi nhap --(lexer)--> token --(Shunting-yard)--> hau to (RPN)
 *             --(stack)--> gia tri
 *
 * Ho tro:
 *   - Toan tu: + - * / % ^ (luy thua, ket hop phai), ! (giai thua), dau am mot ngoi
 *   - Dau ngoac, hang so pi, e, bien ans (ket qua truoc do)
 *   - Ham: sqrt sin cos tan ln log exp abs
 *   - Gan bien: x = 3 * 4
 *
 * Module nay khong dung thu vien chuan C nen bien dich duoc sang WebAssembly.
 */
#ifndef CALC_H
#define CALC_H

#define CALC_MAX_INPUT 255
#define CALC_MAX_VARS  32
#define CALC_NAME_LEN  16

typedef enum {
    CALC_OK = 0,
    CALC_ERR_SYNTAX,
    CALC_ERR_PAREN,
    CALC_ERR_UNKNOWN,
    CALC_ERR_DIV_ZERO,
    CALC_ERR_MOD_ZERO,
    CALC_ERR_DOMAIN,
    CALC_ERR_OVERFLOW,
    CALC_ERR_TOO_LONG,
    CALC_ERR_EMPTY,
    CALC_ERR_READONLY,
    CALC_ERR_TOO_MANY_VARS
} CalcError;

typedef struct {
    char   name[CALC_NAME_LEN];
    double value;
} CalcVar;

/* Moi truong tinh toan: giu bien va ket qua truoc giua cac lan tinh. */
typedef struct {
    CalcVar vars[CALC_MAX_VARS];
    int     var_count;
    double  ans;
} CalcEnv;

typedef struct {
    CalcError error;
    double    value;
    int       pos;                       /* vi tri (tinh tu 0) gay loi, -1 neu khong ro */
    char      assigned[CALC_NAME_LEN];   /* ten bien vua gan, rong neu khong phai phep gan */
} CalcResult;

void        calc_env_init(CalcEnv *env);
CalcResult  calc_eval(CalcEnv *env, const char *expr);
const char *calc_error_message(CalcError err);

#endif
