/* SPDX-License-Identifier: MIT
   Copyright (c) 2026 dIHlHIb
   Developed with AI assistance. */
/* Experimental binary32 nth root for IEEE-754 platforms with dynamic rounding. */
#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "rootnf.h"
#include "rootnf_tables.h"
#include "exact_table.h"
#include "power_interval.h"

static float as_float(uint32_t u) {
    float x;
    memcpy(&x, &u, sizeof(x));
    return x;
}
static uint64_t double_bits(double x) {
    uint64_t u;
    memcpy(&u, &x, sizeof(u));
    return u;
}
static double from_bits(uint64_t u) {
    double x;
    memcpy(&x, &u, sizeof(x));
    return x;
}

static uint32_t exact_word(uint32_t mant, int e, long long n) {
    uint32_t sig = 0x800000u | mant;
    int zeros = __builtin_ctz(sig);
    uint32_t odd = sig >> zeros;
    int xe = e - 23 + zeros;
    uint32_t r = 1;
    if (odd != 1) {
        if (n < 3 || n > 15) return 0;
        uint32_t key = ((uint32_t)n << 24) | odd;
        size_t i = (key * UINT32_C(2654435761)) >> 23;
        while (root_keys[i] != key) {
            if (!root_keys[i]) return 0;
            i = (i+1) & 511u;
        }
        r = root_odds[i];
    }
    if (xe && ((n > 149 || n < -149) || xe % (int)n)) return 0;
    int exp = xe ? xe / (int)n : 0;
    int lead = 31 - __builtin_clz(r);
    return ((uint32_t)(exp + lead + 127) << 23) | ((r << (23-lead)) & 0x7fffffu);
}

static float near_one(uint32_t mag, int negative, long long n) {
    int above = (mag > 0x3f800000u) == (n > 0);
    double proxy = above ? 0x1.0000001p0 : 0x1.ffffffep-1;
    return (float)(negative ? -proxy : proxy);
}

float cr_rootnf(float x, long long n) {
    uint32_t bits;
    memcpy(&bits, &x, sizeof(bits));
    uint32_t sign = bits & 0x80000000u, mag = bits ^ sign;
    if (!n || (sign && mag && mag <= 0x7f800000u && !(n & 1))) {
#ifdef CORE_MATH_SUPPORT_ERRNO
        errno = EDOM;
#endif
        feraiseexcept(FE_INVALID);
        return as_float(0x7fc00000u);
    }
    if (mag > 0x7f800000u) return x + x;
    if (mag == 0x7f800000u) return n > 0 ? x : as_float(sign);
    if (mag == 0) {
        uint32_t s = (n & 1) ? sign : 0;
        if (n > 0) return as_float(s);
#ifdef CORE_MATH_SUPPORT_ERRNO
        errno = ERANGE;
#endif
        feraiseexcept(FE_DIVBYZERO);
        return as_float(s | 0x7f800000u);
    }
    if (n == 1 || mag == 0x3f800000u) return x;
    if (n == -1) {
        float answer = 1.0f / x;
#ifdef CORE_MATH_SUPPORT_ERRNO
        if (mag <= 0x00200000u || (mag > 0x7e800000u && (mag & 0x7fffffu)))
            errno = ERANGE;
#endif
        return answer;
    }
    if (n == 2) return sqrtf(x);
    if (n >= 0x100000000LL || n <= -0x100000000LL)
        return near_one(mag, sign != 0, n);
    int e = (int)(mag >> 23) - 127;
    uint32_t mant = mag & 0x7fffffu;
    if (e == -127) {
        int shift = __builtin_clz(mant) - 8;
        mant = (mant << shift) & 0x7fffffu;
        e = -126 - shift;
    }
    if (!mant || (n >= 3 && n <= 15)) {
        uint32_t exact = exact_word(mant, e, n);
        if (exact) return as_float(sign | exact);
    }
    double reciprocal = 1.0 / (double)n;
    double m = from_bits((UINT64_C(0x3ff) << 52) | ((uint64_t)mant << 29));
    unsigned i = mant >> 16;
    double r = m * LOG_INV[i] - 1.0;
    double r2 = r * r;
    double p = fma(r2 * r2, fma(r, LOGP[5], LOGP[4]),
                   fma(r2, fma(r, LOGP[3], LOGP[2]), fma(r, LOGP[1], LOGP[0])));
    double t = ((double)e + fma(r, p, LOG_T[i][0])) * reciprocal;
    double k = __builtin_roundeven(t * 64.0);
    double f = fma(k, -0x1p-6, t);
    double f2 = f * f;
    double q = fma(f2 * f2, fma(f, EXPQ[4], EXPQ[3]),
                   fma(f2, fma(f, EXPQ[2], EXPQ[1]), f * EXPQ[0]));
    int64_t ki = (int64_t)k;
    double tab = EXP_T[ki & 63][0];
    uint64_t uy = double_bits(fma(tab, q, tab)) + ((uint64_t)(ki >> 6) << 52);
    double y = from_bits(uy);
    uint32_t low = (uint32_t)uy & 0xfffffffu;
    const uint32_t guard = 8192;
    if (__builtin_expect(low <= guard || low >= (1u << 28)-guard, 0)) {
        if (fabs(y - 1.0) < 0x1p-27) return near_one(mag, sign != 0, n);
        uint64_t grid = (uy + (UINT64_C(1)<<27)) & ~((UINT64_C(1)<<28)-1);
        int side;
        if (!root_grid_side(double_bits((double)as_float(mag)), n, grid, &side)) {
#ifdef CORE_MATH_SUPPORT_ERRNO
            errno = ENOMEM;
#endif
            return as_float(0x7fc00000u);
        }
        /* Adjacent doubles are in the immediately adjacent binary32 cells. */
        y = from_bits(grid + (uint64_t)side);
    }
    return (float)(sign ? -y : y);
}
