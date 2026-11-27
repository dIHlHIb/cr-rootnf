// ddlib.h - double-double log2 / exp2 helpers for the rootnf hard-case search.
// Round-to-nearest only. The scanner uses dd_log2 to build its log table;
// audit/table_certificate.c bounds the error of that table below 2^-105.
#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "rootnf_tables.h"
#include "search_tables.h"

static inline void dd_two_sum(double a, double b, double *s, double *e) {
  double t = a + b, bb = t - a;
  *s = t;
  *e = (a - (t - bb)) + (b - bb);
}
static inline void dd_fast_two_sum(double a, double b, double *s, double *e) {
  double t = a + b;
  *s = t;
  *e = b - (t - a);
}
static inline void dd_mul2(double ah, double al, double bh, double bl, double *h, double *l) {
  double p = ah * bh, e = fma(ah, bh, -p);
  e += ah * bl + al * bh;
  dd_fast_two_sum(p, e, h, l);
}
static inline void dd_add2(double ah, double al, double bh, double bl, double *h, double *l) {
  double s, e;
  dd_two_sum(ah, bh, &s, &e);
  e += al + bl;
  dd_fast_two_sum(s, e, h, l);
}

// log2 of a positive double with at most 25 significant bits
static void dd_log2(double v, double *h, double *l) {
  int e;
  double m = frexp(v, &e);
  m *= 2;
  e -= 1;
  int i = (int) ((m - 1.0) * 128.0);
  double r = m * LOG_INV[i] - 1.0;  // exact
  double sh = LOGC_DD[15][0], sl = LOGC_DD[15][1];
  for (int k = 14; k >= 0; k--) {
    dd_mul2(sh, sl, r, 0.0, &sh, &sl);
    dd_add2(sh, sl, LOGC_DD[k][0], LOGC_DD[k][1], &sh, &sl);
  }
  dd_mul2(sh, sl, r, 0.0, &sh, &sl);
  dd_mul2(sh, sl, INVLN2_DD[0], INVLN2_DD[1], &sh, &sl);
  dd_add2(sh, sl, LOG_T[i][0], LOG_T[i][1], &sh, &sl);
  dd_add2((double) e, 0.0, sh, sl, h, l);
}

// 2^(th + tl) as double-double, |th| < 1000; result = (h + l) * 2^(*ex)
// with h + l in [2^-1/64, 2) roughly
static void dd_exp2(double th, double tl, double *h, double *l, int *ex) {
  double k = __builtin_roundeven(th * 64.0);
  double fh, fl;
  dd_two_sum(th - k / 64.0, tl, &fh, &fl);  // th - k/64 exact
  // 2^f = exp(f ln2): series in double-double up to degree 12
  double gh, gl;
  dd_mul2(fh, fl, LN2_DD[0], LN2_DD[1], &gh, &gl);  // g = f ln2, |g| < 2^-7.5
  double sh = 1.0 / 479001600.0, sl = 0.0;            // 1/12!
  for (int j = 11; j >= 1; j--) {
    dd_mul2(sh, sl, gh, gl, &sh, &sl);
    dd_add2(sh, sl, INVFACT_DD[j][0], INVFACT_DD[j][1], &sh, &sl);
  }
  dd_mul2(sh, sl, gh, gl, &sh, &sl);
  dd_add2(1.0, 0.0, sh, sl, &sh, &sl);
  int64_t ki = (int64_t) k;
  int j = (int) (ki & 63);
  dd_mul2(sh, sl, EXP_T[j][0], EXP_T[j][1], h, l);
  *ex = (int) (ki >> 6);
}
