/*
 * test_calc.c - Unit test cho mathx, calc va history.
 * Chay: make test
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "calc.h"
#include "history.h"
#include "mathx.h"

static int passed = 0, failed = 0;

#define CHECK(cond, ...)                                   \
    do {                                                   \
        if (cond) passed++;                                \
        else {                                             \
            failed++;                                      \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__);  \
            printf(__VA_ARGS__);                           \
            printf("\n");                                  \
        }                                                  \
    } while (0)

static double rel_err(double got, double want) {
    double scale = fabs(want) > 1 ? fabs(want) : 1;
    return fabs(got - want) / scale;
}

/* ------------------------------------------------------------------ */

static void test_math_vs_libm(void) {
    double x, worst;

    worst = 0;
    for (x = -700; x <= 700; x += 0.37) { double e = rel_err(mx_exp(x), exp(x)); if (e > worst) worst = e; }
    CHECK(worst < 1e-14, "exp: sai so lon nhat %g", worst);

    worst = 0;
    for (x = 1e-300; x < 1e300; x *= 1.7) { double e = rel_err(mx_ln(x), log(x)); if (e > worst) worst = e; }
    CHECK(worst < 1e-14, "ln: sai so lon nhat %g", worst);

    worst = 0;
    for (x = 1e-300; x < 1e300; x *= 1.7) { double e = rel_err(mx_sqrt(x), sqrt(x)); if (e > worst) worst = e; }
    CHECK(worst < 1e-15, "sqrt: sai so lon nhat %g", worst);

    worst = 0;
    for (x = -1000; x <= 1000; x += 0.013) {
        double e1 = rel_err(mx_sin(x), sin(x)), e2 = rel_err(mx_cos(x), cos(x));
        if (e1 > worst) worst = e1;
        if (e2 > worst) worst = e2;
    }
    CHECK(worst < 1e-13, "sin/cos: sai so lon nhat %g", worst);

    worst = 0;
    for (x = -20; x <= 20; x += 0.0137) { double e = rel_err(mx_pow(2.7, x), pow(2.7, x)); if (e > worst) worst = e; }
    CHECK(worst < 1e-13, "pow: sai so lon nhat %g", worst);
}

static void test_math_special(void) {
    CHECK(mx_ipow(2, 10) == 1024, "2^10");
    CHECK(mx_ipow(2, -2) == 0.25, "2^-2");
    CHECK(mx_ipow(-3, 3) == -27, "(-3)^3");
    CHECK(rel_err(mx_ipow(1.0000001, 2000000000LL), pow(1.0000001, 2e9)) < 1e-6, "luy thua so mu lon");
    CHECK(mx_factorial(0) == 1 && mx_factorial(5) == 120, "giai thua");
    CHECK(mx_isinf(mx_factorial(171)), "171! tran so");
    CHECK(mx_isnan(mx_factorial(2.5)) && mx_isnan(mx_factorial(-1)), "giai thua so khong hop le");
    CHECK(mx_isnan(mx_sqrt(-1)) && mx_isnan(mx_ln(-1)), "mien xac dinh");
    CHECK(mx_fmod(-7, 3) == -1 && mx_fmod(7.5, 2) == 1.5, "fmod");
    CHECK(mx_floor(-2.5) == -3 && mx_floor(2.5) == 2, "floor");
}

/* ------------------------------------------------------------------ */

static void expect_value(CalcEnv *env, const char *expr, double want) {
    CalcResult r = calc_eval(env, expr);
    CHECK(r.error == CALC_OK && rel_err(r.value, want) < 1e-12,
          "'%s' -> loi=%d gia tri=%.15g, mong doi %.15g", expr, r.error, r.value, want);
}

static void expect_error(CalcEnv *env, const char *expr, CalcError want, int want_pos) {
    CalcResult r = calc_eval(env, expr);
    CHECK(r.error == want && (want_pos < 0 || r.pos == want_pos),
          "'%s' -> loi=%d pos=%d, mong doi loi=%d pos=%d", expr, r.error, r.pos, want, want_pos);
}

static void test_parser(void) {
    CalcEnv env;
    calc_env_init(&env);

    /* Do uu tien va ket hop */
    expect_value(&env, "2 + 3 * 4", 14);
    expect_value(&env, "(2 + 3) * 4", 20);
    expect_value(&env, "10 - 4 - 3", 3);          /* ket hop trai */
    expect_value(&env, "100 / 10 / 5", 2);
    expect_value(&env, "2 ^ 3 ^ 2", 512);         /* ket hop phai: 2^9 */
    expect_value(&env, "-2 ^ 2", -4);
    expect_value(&env, "(-2) ^ 2", 4);
    expect_value(&env, "2 ^ -1", 0.5);
    expect_value(&env, "2 * -3", -6);
    expect_value(&env, "--3", 3);
    expect_value(&env, "+5", 5);
    expect_value(&env, "3!", 6);
    expect_value(&env, "-3!", -6);
    expect_value(&env, "(1 + 2)!", 6);
    expect_value(&env, "2 ^ 3!", 64);
    expect_value(&env, "7 % 3", 1);
    expect_value(&env, "-7 % 3", -1);
    expect_value(&env, "((((1))))", 1);

    /* So */
    expect_value(&env, ".5", 0.5);
    expect_value(&env, "1.25e2", 125);
    expect_value(&env, "1e-3", 0.001);
    expect_value(&env, "0.1 + 0.2", 0.3);

    /* Ham va hang so */
    expect_value(&env, "sqrt(16)", 4);
    expect_value(&env, "sin(pi / 2)", 1);
    expect_value(&env, "cos(0)", 1);
    expect_value(&env, "ln(e)", 1);
    expect_value(&env, "log(1000)", 3);
    expect_value(&env, "exp(0)", 1);
    expect_value(&env, "abs(-3.5)", 3.5);
    expect_value(&env, "sqrt(sqrt(16))", 2);
    expect_value(&env, "2 ^ 0.5", sqrt(2));

    /* Bien va ans */
    expect_value(&env, "r = 5", 5);
    expect_value(&env, "pi * r ^ 2", MX_PI * 25);
    expect_value(&env, "ans / pi", 25);
    expect_value(&env, "r = r + 1", 6);
    expect_value(&env, "r", 6);
    CHECK(env.var_count == 1, "chi co 1 bien r, co %d", env.var_count);
}

static void test_errors(void) {
    CalcEnv env;
    calc_env_init(&env);

    expect_error(&env, "",          CALC_ERR_EMPTY, -1);
    expect_error(&env, "   ",       CALC_ERR_EMPTY, -1);
    expect_error(&env, "1 / 0",     CALC_ERR_DIV_ZERO, 2);
    expect_error(&env, "5 % 0",     CALC_ERR_MOD_ZERO, 2);
    expect_error(&env, "0 ^ -1",    CALC_ERR_DIV_ZERO, 2);
    expect_error(&env, "sqrt(-1)",  CALC_ERR_DOMAIN, 0);
    expect_error(&env, "ln(0)",     CALC_ERR_DOMAIN, 0);
    expect_error(&env, "(-8) ^ 0.5", CALC_ERR_DOMAIN, -1);
    expect_error(&env, "tan(pi/2)", CALC_ERR_DOMAIN, -1);
    expect_error(&env, "2.5!",      CALC_ERR_DOMAIN, 3);
    expect_error(&env, "171!",      CALC_ERR_OVERFLOW, -1);
    expect_error(&env, "10 ^ 400",  CALC_ERR_OVERFLOW, -1);
    expect_error(&env, "1e999",     CALC_ERR_OVERFLOW, 0);
    expect_error(&env, "2 3",       CALC_ERR_SYNTAX, 2);
    expect_error(&env, "2 +",       CALC_ERR_SYNTAX, -1);
    expect_error(&env, "* 2",       CALC_ERR_SYNTAX, 0);
    expect_error(&env, "2 (3)",     CALC_ERR_SYNTAX, 2);
    expect_error(&env, "()",        CALC_ERR_SYNTAX, 1);
    expect_error(&env, "!3",        CALC_ERR_SYNTAX, 0);
    expect_error(&env, "2 # 3",     CALC_ERR_SYNTAX, 2);
    expect_error(&env, "sqrt 4",    CALC_ERR_SYNTAX, 0);
    expect_error(&env, "(2 + 3",    CALC_ERR_PAREN, 0);
    expect_error(&env, "2 + 3)",    CALC_ERR_PAREN, 5);
    expect_error(&env, "foo + 1",   CALC_ERR_UNKNOWN, 0);
    expect_error(&env, "pi = 3",    CALC_ERR_READONLY, 0);
    expect_error(&env, "sin = 3",   CALC_ERR_READONLY, 0);
    expect_error(&env, "x =",       CALC_ERR_SYNTAX, -1);
    expect_error(&env, "2 = 3",     CALC_ERR_SYNTAX, -1);
    expect_error(&env, "averyveryverylongname", CALC_ERR_TOO_LONG, 0);

    /* Loi khong duoc lam thay doi ans */
    calc_eval(&env, "42");
    calc_eval(&env, "1 / 0");
    CHECK(env.ans == 42, "ans bi thay doi sau loi: %g", env.ans);
}

static void test_limits(void) {
    CalcEnv env;
    char buf[CALC_MAX_INPUT + 10];
    char name[16];
    int i;
    calc_env_init(&env);

    /* 255 ky tu: van hop le; 256: qua dai */
    memset(buf, 0, sizeof buf);
    buf[0] = '1';
    for (i = 1; i + 1 < CALC_MAX_INPUT; i += 2) { buf[i] = '+'; buf[i + 1] = '1'; }
    CHECK(calc_eval(&env, buf).error == CALC_OK, "bieu thuc dai %d ky tu", (int)strlen(buf));
    memset(buf, '1', CALC_MAX_INPUT + 1);
    buf[CALC_MAX_INPUT + 1] = '\0';
    CHECK(calc_eval(&env, buf).error == CALC_ERR_TOO_LONG, "bieu thuc qua dai");

    /* Ngoac long nhau sau toi da */
    memset(buf, 0, sizeof buf);
    for (i = 0; i < 100; i++) buf[i] = '(';
    buf[100] = '1';
    for (i = 101; i < 201; i++) buf[i] = ')';
    CHECK(calc_eval(&env, buf).error == CALC_OK, "100 tang ngoac");

    /* Het cho luu bien */
    for (i = 0; i < CALC_MAX_VARS; i++) {
        sprintf(name, "v%d", i);
        sprintf(buf, "%s = %d", name, i);
        calc_eval(&env, buf);
    }
    CHECK(env.var_count == CALC_MAX_VARS, "so bien %d", env.var_count);
    CHECK(calc_eval(&env, "extra = 1").error == CALC_ERR_TOO_MANY_VARS, "vuot gioi han bien");
    CHECK(calc_eval(&env, "v0 = 99").error == CALC_OK, "gan lai bien cu van duoc khi day");
}

/* ------------------------------------------------------------------ */

static void test_history(void) {
    History h, loaded;
    const char *path = "tests/tmp_history.txt";
    int i;

    history_init(&h);
    CHECK(history_get(&h, 1) == NULL, "lich su rong");
    history_add(&h, "1 + 1", 2);
    history_add(&h, "sqrt(9)", 3);
    CHECK(h.count == 2 && strcmp(history_get(&h, 2)->expr, "sqrt(9)") == 0, "them va lay");

    CHECK(history_save(&h, path), "luu file");
    history_init(&loaded);
    CHECK(history_load(&loaded, path), "doc file");
    CHECK(loaded.count == 2 && history_get(&loaded, 1)->value == 2 &&
          strcmp(history_get(&loaded, 1)->expr, "1 + 1") == 0, "noi dung sau khi doc lai");
    history_clear(&loaded);
    CHECK(loaded.count == 0 && loaded.head == NULL && loaded.tail == NULL, "xoa het");
    remove(path);

    /* Vuot gioi han: phan tu cu nhat bi xoa */
    for (i = 0; i < HISTORY_MAX + 10; i++) history_add(&h, "x", i);
    CHECK(h.count == HISTORY_MAX, "gioi han %d, co %d", HISTORY_MAX, h.count);
    CHECK(history_get(&h, HISTORY_MAX)->value == HISTORY_MAX + 9, "phan tu moi nhat o cuoi");
    history_clear(&h);
}

int main(void) {
    test_math_vs_libm();
    test_math_special();
    test_parser();
    test_errors();
    test_limits();
    test_history();

    printf("\n  %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
