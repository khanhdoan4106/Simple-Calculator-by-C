/*
 * mathx.c - Cai dat cac ham toan hoc khong dung <math.h>.
 *
 * Ky thuat chinh:
 *   - Thao tac truc tiep tren bit IEEE-754 de tach so mu (giong frexp/ldexp).
 *   - Thu gon mien (range reduction) truoc khi dung chuoi Taylor de hoi tu nhanh.
 *   - Tach hang so (ln2, pi/2) thanh phan "hi" + "lo" de giam sai so lam tron.
 */
#include "mathx.h"

typedef union {
    double d;
    unsigned long long u;
} bits64;

/* ---------- Tien ich co ban ---------- */

int mx_isnan(double x) { return x != x; }

int mx_isinf(double x) { return !mx_isnan(x) && mx_isnan(x - x); }

double mx_inf(void) { bits64 b; b.u = 0x7FF0000000000000ULL; return b.d; }

double mx_nan(void) { bits64 b; b.u = 0x7FF8000000000000ULL; return b.d; }

double mx_fabs(double x) { return x < 0 ? -x : x; }

double mx_trunc(double x) {
    /* Moi so >= 2^52 deu da la so nguyen trong double. */
    if (mx_isnan(x) || mx_fabs(x) >= 4503599627370496.0) return x;
    return (double)(long long)x;
}

double mx_floor(double x) {
    double t = mx_trunc(x);
    return (t > x) ? t - 1.0 : t;
}

static double mx_round(double x) { return mx_floor(x + 0.5); }

int mx_is_integer(double x) {
    return !mx_isnan(x) && !mx_isinf(x) && mx_trunc(x) == x;
}

double mx_fmod(double a, double b) {
    if (b == 0 || mx_isnan(a) || mx_isnan(b) || mx_isinf(a)) return mx_nan();
    if (mx_isinf(b)) return a;
    return a - mx_trunc(a / b) * b;
}

/* Tach x = m * 2^e voi m trong [1, 2). Chi dung cho x > 0 huu han. */
static double split_exponent(double x, int *e) {
    bits64 b;
    int adjust = 0;
    b.d = x;
    if (((b.u >> 52) & 0x7FF) == 0) {      /* so subnormal: phong to truoc */
        b.d = x * 18014398509481984.0;     /* 2^54 */
        adjust = -54;
    }
    *e = (int)((b.u >> 52) & 0x7FF) - 1023 + adjust;
    b.u = (b.u & 0x800FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    return b.d;
}

/* Tinh 2^k bang cach dung truc tiep cac bit so mu. */
static double pow2i(int k) {
    bits64 b;
    if (k > 1023) return mx_inf();
    if (k < -1074) return 0.0;
    if (k < -1022) return pow2i(k + 54) / 18014398509481984.0;
    b.u = (unsigned long long)(k + 1023) << 52;
    return b.d;
}

/* ---------- Luy thua nhanh ---------- */

double mx_ipow(double base, long long exp) {
    double result = 1.0;
    int negative = exp < 0;
    unsigned long long n = negative ? (unsigned long long)(-(exp + 1)) + 1ULL
                                    : (unsigned long long)exp;
    /* Binh phuong lien tiep: base^13 = base^8 * base^4 * base^1 (13 = 1101b) */
    while (n > 0) {
        if (n & 1ULL) result *= base;
        base *= base;
        n >>= 1;
    }
    return negative ? 1.0 / result : result;
}

/* ---------- Can bac hai: Newton-Raphson ---------- */

double mx_sqrt(double x) {
    int e, i;
    double guess, next;
    if (mx_isnan(x) || x < 0) return mx_nan();
    if (x == 0 || mx_isinf(x)) return x;

    /* Doan ban dau tot: chia doi so mu -> chi can ~5 vong lap thay vi hang tram. */
    split_exponent(x, &e);
    guess = pow2i(e / 2);

    for (i = 0; i < 100; i++) {
        next = 0.5 * (guess + x / guess);
        if (mx_fabs(next - guess) <= 1e-16 * next) { guess = next; break; }
        guess = next;
    }
    return guess;
}

/* ---------- Ham mu e^x ---------- */

static const double LN2_HI = 6.93147180369123816490e-01;
static const double LN2_LO = 1.90821492927058770002e-10;

double mx_exp(double x) {
    double k, r, term, sum;
    int n;
    if (mx_isnan(x)) return x;
    if (x > 709.782712893384) return mx_inf();
    if (x < -745.1332191019411) return 0.0;

    /* x = k*ln2 + r, |r| <= ln2/2  =>  e^x = 2^k * e^r */
    k = mx_round(x / MX_LN2);
    r = (x - k * LN2_HI) - k * LN2_LO;

    sum = 1.0;
    term = 1.0;
    for (n = 1; n < 30; n++) {
        term *= r / n;
        sum += term;
        if (mx_fabs(term) < 1e-17 * sum) break;
    }
    /* Tach 2^k thanh hai lan nhan de khong tran so o bien. */
    if (k > 1000) return sum * pow2i(1000) * pow2i((int)k - 1000);
    if (k < -1000) return sum * pow2i(-1000) * pow2i((int)k + 1000);
    return sum * pow2i((int)k);
}

/* ---------- Logarit tu nhien ---------- */

double mx_ln(double x) {
    int e, n;
    double m, z, z2, term, sum;
    if (mx_isnan(x) || x < 0) return mx_nan();
    if (x == 0) return -mx_inf();
    if (mx_isinf(x)) return x;

    /* x = m * 2^e, dua m ve [sqrt(2)/2, sqrt(2)) de chuoi hoi tu nhanh */
    m = split_exponent(x, &e);
    if (m > 1.4142135623730951) { m *= 0.5; e++; }

    /* ln m = 2 * (z + z^3/3 + z^5/5 + ...), z = (m-1)/(m+1), |z| < 0.172 */
    z = (m - 1.0) / (m + 1.0);
    z2 = z * z;
    term = z;
    sum = 0.0;
    for (n = 1; n < 60; n += 2) {
        sum += term / n;
        term *= z2;
        if (mx_fabs(term) < 1e-18) break;
    }
    return 2.0 * sum + e * LN2_HI + e * LN2_LO;
}

double mx_log10(double x) { return mx_ln(x) / 2.30258509299404568402; }

/* ---------- Luy thua so mu thuc ---------- */

double mx_pow(double base, double exp) {
    if (mx_isnan(base) || mx_isnan(exp)) return mx_nan();
    if (exp == 0) return 1.0;
    if (mx_is_integer(exp) && mx_fabs(exp) < 9.2e18)
        return mx_ipow(base, (long long)exp);
    if (base < 0) return mx_nan();      /* (-8)^(1/3) khong co nghiem thuc theo cach nay */
    if (base == 0) return exp > 0 ? 0.0 : mx_inf();
    return mx_exp(exp * mx_ln(base));
}

/* ---------- Luong giac ---------- */

static const double PIO2_HI = 1.57079632673412561417e+00;
static const double PIO2_LO = 6.07710050650619224932e-11;

/* Chuoi Taylor tren |t| <= pi/4 */
static double kernel_sin(double t) {
    double t2 = t * t, term = t, sum = t;
    int n;
    for (n = 3; n < 40; n += 2) {
        term *= -t2 / ((n - 1) * n);
        sum += term;
        if (mx_fabs(term) < 1e-18) break;
    }
    return sum;
}

static double kernel_cos(double t) {
    double t2 = t * t, term = 1.0, sum = 1.0;
    int n;
    for (n = 2; n < 40; n += 2) {
        term *= -t2 / ((n - 1) * n);
        sum += term;
        if (mx_fabs(term) < 1e-18) break;
    }
    return sum;
}

/* Dua x ve t trong [-pi/4, pi/4] va goc phan tu q (mod 4). */
static double reduce_quadrant(double x, int *q) {
    double k = mx_round(x * 0.63661977236758134308);   /* x / (pi/2) */
    long long kq = (long long)mx_fmod(k, 4.0);
    if (kq < 0) kq += 4;
    *q = (int)kq;
    return (x - k * PIO2_HI) - k * PIO2_LO;
}

double mx_sin(double x) {
    int q;
    double t;
    if (mx_isnan(x) || mx_isinf(x)) return mx_nan();
    t = reduce_quadrant(x, &q);
    switch (q) {
        case 0:  return kernel_sin(t);
        case 1:  return kernel_cos(t);
        case 2:  return -kernel_sin(t);
        default: return -kernel_cos(t);
    }
}

double mx_cos(double x) {
    int q;
    double t;
    if (mx_isnan(x) || mx_isinf(x)) return mx_nan();
    t = reduce_quadrant(x, &q);
    switch (q) {
        case 0:  return kernel_cos(t);
        case 1:  return -kernel_sin(t);
        case 2:  return -kernel_cos(t);
        default: return kernel_sin(t);
    }
}

double mx_tan(double x) { return mx_sin(x) / mx_cos(x); }

/* ---------- Giai thua ---------- */

double mx_factorial(double n) {
    double result = 1.0;
    int i;
    if (!mx_is_integer(n) || n < 0) return mx_nan();
    if (n > 170) return mx_inf();   /* 171! vuot qua gioi han cua double */
    for (i = 2; i <= (int)n; i++) result *= i;
    return result;
}
