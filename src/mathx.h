/*
 * mathx.h - Cac ham toan hoc tu cai dat, KHONG dung <math.h>.
 *
 * Moi ham deu "freestanding" (khong can thu vien chuan C) de co the
 * bien dich ca sang WebAssembly cho ban demo web.
 */
#ifndef MATHX_H
#define MATHX_H

#define MX_PI  3.14159265358979323846
#define MX_E   2.71828182845904523536
#define MX_LN2 0.69314718055994530942

/* Kiem tra NaN / vo cung ma khong can <math.h> */
int    mx_isnan(double x);
int    mx_isinf(double x);
double mx_inf(void);
double mx_nan(void);

double mx_fabs(double x);
double mx_trunc(double x);          /* lam tron ve 0 */
double mx_floor(double x);
double mx_fmod(double a, double b); /* phan du, cung dau voi a */
int    mx_is_integer(double x);

/* Luy thua nhanh (chia de tri): O(log n) phep nhan thay vi O(n). */
double mx_ipow(double base, long long exp);

double mx_sqrt(double x);           /* Newton-Raphson */
double mx_exp(double x);            /* chuoi Taylor + thu gon mien 2^k */
double mx_ln(double x);             /* chuoi atanh: ln m = 2*atanh((m-1)/(m+1)) */
double mx_log10(double x);
double mx_pow(double base, double exp); /* so mu thuc: e^(b*ln a) */
double mx_sin(double x);            /* chuoi Taylor sau khi dua ve [-pi, pi] */
double mx_cos(double x);
double mx_tan(double x);
double mx_factorial(double n);      /* n! voi n nguyen, 0 <= n <= 170 */

#endif
