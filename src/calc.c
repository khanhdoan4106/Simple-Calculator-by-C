/*
 * calc.c - Lexer + thuat toan Shunting-yard (Dijkstra) + tinh gia tri RPN.
 *
 * Khong dung malloc hay <string.h>: moi mang co kich thuoc co dinh, du cho
 * bieu thuc dai toi da CALC_MAX_INPUT ky tu.
 */
#include "calc.h"
#include "mathx.h"

/* ====================================================================
 * Tien ich chuoi (thay cho <string.h> / <ctype.h>)
 * ==================================================================== */

static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int is_space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void str_copy(char *dst, const char *src, int cap) {
    int i = 0;
    while (src[i] && i < cap - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* ====================================================================
 * Lexer: tach chuoi thanh token
 * ==================================================================== */

typedef enum { T_NUM, T_IDENT, T_OP, T_LPAREN, T_RPAREN, T_FACT, T_ASSIGN } TokType;

typedef struct {
    TokType type;
    double  num;
    char    op;                    /* + - * / % ^ */
    char    name[CALC_NAME_LEN];
    int     pos;
} Token;

/* Doc mot so dang 12, 3.5, .5, 1e-3, 2.5E+10 */
static int lex_number(const char *s, int i, double *out) {
    double mant = 0.0;
    int frac_digits = 0, exp10 = 0, seen_digit = 0;

    while (is_digit(s[i])) { mant = mant * 10.0 + (s[i] - '0'); i++; seen_digit = 1; }
    if (s[i] == '.') {
        i++;
        while (is_digit(s[i])) { mant = mant * 10.0 + (s[i] - '0'); frac_digits++; i++; seen_digit = 1; }
    }
    if (!seen_digit) return -1;

    /* Phan mu: chi nhan khi sau 'e' thuc su la chu so, de "2e" khong nuot hang so e */
    if ((s[i] == 'e' || s[i] == 'E') &&
        (is_digit(s[i + 1]) || ((s[i + 1] == '+' || s[i + 1] == '-') && is_digit(s[i + 2])))) {
        int sign = 1, e = 0;
        i++;
        if (s[i] == '+' || s[i] == '-') { sign = (s[i] == '-') ? -1 : 1; i++; }
        while (is_digit(s[i])) { if (e < 10000) e = e * 10 + (s[i] - '0'); i++; }
        exp10 = sign * e;
    }

    exp10 -= frac_digits;
    /* Chia cho 10^n chinh xac hon nhan voi 10^-n (vi 10^-n khong bieu dien dung). */
    if (exp10 >= 0)           *out = mant * mx_ipow(10.0, exp10);
    else if (exp10 >= -308)   *out = mant / mx_ipow(10.0, -exp10);
    else                      *out = (mant / mx_ipow(10.0, 308)) / mx_ipow(10.0, -exp10 - 308);
    return i;
}

static CalcError tokenize(const char *s, Token *toks, int *count, int *err_pos) {
    int i = 0, n = 0;
    while (s[i]) {
        char c = s[i];
        Token *t = &toks[n];
        if (is_space(c)) { i++; continue; }
        if (n >= CALC_MAX_INPUT) { *err_pos = i; return CALC_ERR_TOO_LONG; }
        t->pos = i;

        if (is_digit(c) || (c == '.' && is_digit(s[i + 1]))) {
            int next = lex_number(s, i, &t->num);
            if (next < 0) { *err_pos = i; return CALC_ERR_SYNTAX; }
            t->type = T_NUM;
            i = next;
        } else if (is_alpha(c)) {
            int len = 0;
            while (is_alpha(s[i]) || is_digit(s[i])) {
                if (len >= CALC_NAME_LEN - 1) { *err_pos = t->pos; return CALC_ERR_TOO_LONG; }
                t->name[len++] = s[i++];
            }
            t->name[len] = '\0';
            t->type = T_IDENT;
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^') {
            t->type = T_OP; t->op = c; i++;
        } else if (c == '(') { t->type = T_LPAREN; i++; }
        else if (c == ')')   { t->type = T_RPAREN; i++; }
        else if (c == '!')   { t->type = T_FACT;   i++; }
        else if (c == '=')   { t->type = T_ASSIGN; i++; }
        else { *err_pos = i; return CALC_ERR_SYNTAX; }
        n++;
    }
    *count = n;
    return CALC_OK;
}

/* ====================================================================
 * Ham va hang so
 * ==================================================================== */

typedef enum { F_SQRT, F_SIN, F_COS, F_TAN, F_LN, F_LOG, F_EXP, F_ABS, F_NONE } FuncId;

static const char *FUNC_NAMES[] = { "sqrt", "sin", "cos", "tan", "ln", "log", "exp", "abs" };

static FuncId find_func(const char *name) {
    int i;
    for (i = 0; i < F_NONE; i++)
        if (str_eq(name, FUNC_NAMES[i])) return (FuncId)i;
    return F_NONE;
}

static int is_reserved(const char *name) {
    return find_func(name) != F_NONE || str_eq(name, "pi") || str_eq(name, "e") || str_eq(name, "ans");
}

static CalcVar *find_var(CalcEnv *env, const char *name) {
    int i;
    for (i = 0; i < env->var_count; i++)
        if (str_eq(env->vars[i].name, name)) return &env->vars[i];
    return 0;
}

static int lookup_value(CalcEnv *env, const char *name, double *out) {
    CalcVar *v;
    if (str_eq(name, "pi"))  { *out = MX_PI; return 1; }
    if (str_eq(name, "e"))   { *out = MX_E;  return 1; }
    if (str_eq(name, "ans")) { *out = env->ans; return 1; }
    v = find_var(env, name);
    if (v) { *out = v->value; return 1; }
    return 0;
}

/* ====================================================================
 * Shunting-yard: chuyen trung to -> hau to (RPN)
 * ==================================================================== */

typedef enum { R_NUM, R_BINOP, R_NEG, R_FACT, R_FUNC, R_LPAREN } RpnKind;

typedef struct {
    RpnKind kind;
    double  num;
    char    op;
    FuncId  func;
    int     pos;
} RpnItem;

/* Do uu tien: + - < * / % < dau am mot ngoi < ^.  Vi vay -2^2 = -(2^2) = -4. */
static int precedence(const RpnItem *it) {
    if (it->kind == R_NEG) return 3;
    switch (it->op) {
        case '+': case '-': return 1;
        case '*': case '/': case '%': return 2;
        case '^': return 4;
    }
    return 0;
}

static int right_assoc(const RpnItem *it) { return it->kind == R_NEG || it->op == '^'; }

static CalcError to_rpn(CalcEnv *env, const Token *toks, int n,
                        RpnItem *out, int *out_n, int *err_pos) {
    RpnItem stack[CALC_MAX_INPUT];
    int sp = 0, on = 0, i;
    int expect_operand = 1;   /* dang cho mot toan hang (so, bien, '(' hoac dau am)? */

    for (i = 0; i < n; i++) {
        const Token *t = &toks[i];
        *err_pos = t->pos;

        switch (t->type) {
        case T_NUM:
            if (!expect_operand) return CALC_ERR_SYNTAX;          /* "2 3" */
            out[on].kind = R_NUM; out[on].num = t->num; out[on].pos = t->pos; on++;
            expect_operand = 0;
            break;

        case T_IDENT: {
            FuncId f = find_func(t->name);
            if (!expect_operand) return CALC_ERR_SYNTAX;          /* "2 x" */
            if (f != F_NONE) {
                if (i + 1 >= n || toks[i + 1].type != T_LPAREN) return CALC_ERR_SYNTAX;  /* "sqrt 4" */
                stack[sp].kind = R_FUNC; stack[sp].func = f; stack[sp].pos = t->pos; sp++;
            } else {
                double v;
                if (!lookup_value(env, t->name, &v)) return CALC_ERR_UNKNOWN;
                out[on].kind = R_NUM; out[on].num = v; out[on].pos = t->pos; on++;
                expect_operand = 0;
            }
            break;
        }

        case T_OP: {
            RpnItem op;
            op.op = t->op; op.pos = t->pos; op.kind = R_BINOP;
            if (expect_operand) {
                if (t->op == '+') break;                          /* +5 = 5 */
                if (t->op != '-') return CALC_ERR_SYNTAX;         /* "*5", "2+*3" */
                op.kind = R_NEG;
            } else {
                /* Day cac toan tu uu tien cao hon (hoac bang, neu ket hop trai) ra output */
                while (sp > 0 && stack[sp - 1].kind != R_LPAREN && stack[sp - 1].kind != R_FUNC) {
                    int pt = precedence(&stack[sp - 1]), po = precedence(&op);
                    if (pt > po || (pt == po && !right_assoc(&op))) out[on++] = stack[--sp];
                    else break;
                }
            }
            stack[sp++] = op;
            expect_operand = 1;
            break;
        }

        case T_FACT:
            if (expect_operand) return CALC_ERR_SYNTAX;           /* "!3" */
            out[on].kind = R_FACT; out[on].pos = t->pos; on++;    /* hau to: ap dung ngay */
            break;

        case T_LPAREN:
            if (!expect_operand) return CALC_ERR_SYNTAX;          /* "2(3)" - can viet 2*(3) */
            stack[sp].kind = R_LPAREN; stack[sp].pos = t->pos; sp++;
            break;

        case T_RPAREN:
            if (expect_operand) return CALC_ERR_SYNTAX;           /* "()" hoac "(2+)" */
            while (sp > 0 && stack[sp - 1].kind != R_LPAREN) out[on++] = stack[--sp];
            if (sp == 0) return CALC_ERR_PAREN;                   /* thua ')' */
            sp--;                                                  /* bo '(' */
            if (sp > 0 && stack[sp - 1].kind == R_FUNC) out[on++] = stack[--sp];
            break;

        case T_ASSIGN:
            return CALC_ERR_SYNTAX;                               /* '=' chi hop le o dau */
        }
    }

    if (n == 0) return CALC_ERR_EMPTY;
    if (expect_operand) { *err_pos = toks[n - 1].pos; return CALC_ERR_SYNTAX; }  /* "2+" */

    while (sp > 0) {
        if (stack[sp - 1].kind == R_LPAREN) { *err_pos = stack[sp - 1].pos; return CALC_ERR_PAREN; }
        out[on++] = stack[--sp];
    }
    *out_n = on;
    return CALC_OK;
}

/* ====================================================================
 * Tinh gia tri RPN bang stack
 * ==================================================================== */

static CalcError check_value(double v) {
    if (mx_isnan(v)) return CALC_ERR_DOMAIN;
    if (mx_isinf(v)) return CALC_ERR_OVERFLOW;
    return CALC_OK;
}

static CalcError apply_func(FuncId f, double x, double *out) {
    switch (f) {
        case F_SQRT: if (x < 0) return CALC_ERR_DOMAIN; *out = mx_sqrt(x); break;
        case F_LN:   if (x <= 0) return CALC_ERR_DOMAIN; *out = mx_ln(x); break;
        case F_LOG:  if (x <= 0) return CALC_ERR_DOMAIN; *out = mx_log10(x); break;
        case F_SIN:  *out = mx_sin(x); break;
        case F_COS:  *out = mx_cos(x); break;
        case F_TAN:
            /* cos(x) = 0 tai pi/2 + k*pi: tan khong xac dinh */
            if (mx_fabs(mx_cos(x)) < 1e-15) return CALC_ERR_DOMAIN;
            *out = mx_tan(x); break;
        case F_EXP:  *out = mx_exp(x); break;
        case F_ABS:  *out = mx_fabs(x); break;
        default:     return CALC_ERR_SYNTAX;
    }
    return check_value(*out);
}

static CalcError apply_binop(char op, double a, double b, double *out) {
    switch (op) {
        case '+': *out = a + b; break;
        case '-': *out = a - b; break;
        case '*': *out = a * b; break;
        case '/':
            if (b == 0) return CALC_ERR_DIV_ZERO;
            *out = a / b; break;
        case '%':
            if (b == 0) return CALC_ERR_MOD_ZERO;
            *out = mx_fmod(a, b); break;
        case '^':
            if (a == 0 && b < 0) return CALC_ERR_DIV_ZERO;        /* 0^-1 = 1/0 */
            *out = mx_pow(a, b); break;
        default: return CALC_ERR_SYNTAX;
    }
    return check_value(*out);
}

static CalcError eval_rpn(const RpnItem *items, int n, double *result, int *err_pos) {
    double st[CALC_MAX_INPUT];
    int sp = 0, i;
    CalcError e;

    for (i = 0; i < n; i++) {
        const RpnItem *it = &items[i];
        *err_pos = it->pos;
        switch (it->kind) {
        case R_NUM:
            if ((e = check_value(it->num)) != CALC_OK) return e;  /* vi du 1e999 */
            st[sp++] = it->num;
            break;
        case R_NEG:
            st[sp - 1] = -st[sp - 1];
            break;
        case R_FACT:
            st[sp - 1] = mx_factorial(st[sp - 1]);
            if ((e = check_value(st[sp - 1])) != CALC_OK) return e;
            break;
        case R_FUNC:
            if ((e = apply_func(it->func, st[sp - 1], &st[sp - 1])) != CALC_OK) return e;
            break;
        case R_BINOP:
            if ((e = apply_binop(it->op, st[sp - 2], st[sp - 1], &st[sp - 2])) != CALC_OK) return e;
            sp--;
            break;
        default:
            return CALC_ERR_SYNTAX;
        }
    }
    if (sp != 1) return CALC_ERR_SYNTAX;
    /* Tranh in ra "-0" */
    *result = (st[0] == 0) ? 0.0 : st[0];
    return CALC_OK;
}

/* ====================================================================
 * API cong khai
 * ==================================================================== */

void calc_env_init(CalcEnv *env) {
    env->var_count = 0;
    env->ans = 0.0;
}

CalcResult calc_eval(CalcEnv *env, const char *expr) {
    Token toks[CALC_MAX_INPUT];
    RpnItem rpn[CALC_MAX_INPUT];
    CalcResult r;
    int n = 0, rn = 0, start = 0, len = 0;
    const char *target = 0;

    r.error = CALC_OK;
    r.value = 0.0;
    r.pos = -1;
    r.assigned[0] = '\0';

    while (expr[len]) len++;
    if (len > CALC_MAX_INPUT) { r.error = CALC_ERR_TOO_LONG; return r; }

    if ((r.error = tokenize(expr, toks, &n, &r.pos)) != CALC_OK) return r;

    /* Phep gan:  ten = bieu_thuc */
    if (n >= 2 && toks[0].type == T_IDENT && toks[1].type == T_ASSIGN) {
        target = toks[0].name;
        if (is_reserved(target)) { r.error = CALC_ERR_READONLY; r.pos = toks[0].pos; return r; }
        if (!find_var(env, target) && env->var_count >= CALC_MAX_VARS) {
            r.error = CALC_ERR_TOO_MANY_VARS; r.pos = toks[0].pos; return r;
        }
        start = 2;
        if (n == 2) { r.error = CALC_ERR_SYNTAX; r.pos = toks[1].pos; return r; }  /* "x =" */
    }

    if ((r.error = to_rpn(env, toks + start, n - start, rpn, &rn, &r.pos)) != CALC_OK) return r;
    if ((r.error = eval_rpn(rpn, rn, &r.value, &r.pos)) != CALC_OK) return r;

    r.pos = -1;
    if (target) {
        CalcVar *v = find_var(env, target);
        if (!v) {
            v = &env->vars[env->var_count++];
            str_copy(v->name, target, CALC_NAME_LEN);
        }
        v->value = r.value;
        str_copy(r.assigned, target, CALC_NAME_LEN);
    }
    env->ans = r.value;
    return r;
}

const char *calc_error_message(CalcError err) {
    switch (err) {
        case CALC_OK:                return "OK";
        case CALC_ERR_SYNTAX:        return "Bieu thuc khong hop le";
        case CALC_ERR_PAREN:         return "Thieu hoac thua dau ngoac";
        case CALC_ERR_UNKNOWN:       return "Bien hoac ham khong ton tai";
        case CALC_ERR_DIV_ZERO:      return "Khong the chia cho 0";
        case CALC_ERR_MOD_ZERO:      return "Khong the chia lay du cho 0";
        case CALC_ERR_DOMAIN:        return "Gia tri nam ngoai mien xac dinh";
        case CALC_ERR_OVERFLOW:      return "Ket qua qua lon (tran so)";
        case CALC_ERR_TOO_LONG:      return "Bieu thuc hoac ten bien qua dai";
        case CALC_ERR_EMPTY:         return "Chua nhap bieu thuc";
        case CALC_ERR_READONLY:      return "Khong the gan gia tri cho ten nay";
        case CALC_ERR_TOO_MANY_VARS: return "Da het cho luu bien";
    }
    return "Loi khong xac dinh";
}
