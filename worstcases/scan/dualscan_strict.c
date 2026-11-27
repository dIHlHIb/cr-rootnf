// dualscan.c (v2) - dual enumeration for the rootnf hard-case search.
// v2: the nearest-x choice no longer relies on libm exp2 accuracy.
//
// A hard case (x, n, g) - x a positive finite binary32, g a grid point
// (binary32 value or rounding midpoint) close to y = x^(1/n) - has
//   r = log2 x - n log2 g  small,
// hence X = g^n lies within a factor 2^|r| of x and x is the binary32 nearest
// to X. So instead of scanning all 2^31 inputs x for each n, scan, for each n,
// the grid points g whose power X = g^n rounds to a positive finite binary32
// (about 2^24 * 278 / |n| of them), take x = nearest binary32 to X and
// compute r. Every (x, n, g) with |r| < thr is reported, with no gaps, as long
// as the computed r has absolute error below the slack added to thr.
//
// log2 of every 25-bit mantissa 1 + i 2^-24 (i < 2^24) is tabulated once in
// double-double (256 MB). Then t = n log2 g, f = frac(t) and
// r = f - log2(mantissa of x) are computed in double-double; the only
// non-tabulated operation is a double exp2 used to pick the nearest x, which
// is harmless: if X is close to a binary32 the nearest one is found exactly,
// and otherwise |r| is large anyway.
//
// Usage: dualscan n0 n1 Rlog2 outfile
// Output lines: "n xhex e i rhex" with g = 2^e (1 + i 2^-24).
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ddlib.h"

#define TBITS 24
static double *Th, *Tl;  // log2(1 + i 2^-24), double-double

static void build_table(void) {
  size_t n = (size_t) 1 << TBITS;
  Th = malloc(n * 8);
  Tl = malloc(n * 8);
  for (size_t i = 0; i < n; i++) dd_log2(1.0 + (double) i * 0x1p-24, &Th[i], &Tl[i]);
}

static FILE *out;
static uint64_t ncand, npairs;
static double thr;

// log2 of an integer 1 <= m < 2^24 as double-double via the table
static inline void log2_int(uint32_t m, double *h, double *l) {
  int p = 31 - __builtin_clz(m);
  uint32_t i = (m - (1u << p)) << (24 - p);
  dd_fast_two_sum((double) p, Th[i], h, l);
  *l += Tl[i];
}

// (ah + al) < (bh + bl) for double-doubles, decided on (ah - bh) + (al - bl)
static inline int below(double ah, double al, double bh, double bl) {
  return (ah - bh) + (al - bl) < 0.0;
}

static inline void report(long long n, uint32_t xb, int e, uint32_t i, double r) {
  ncand++;
  fprintf(out, "%lld %08x %d %u %a\n", n, xb, e, i, r);
}

// one pair: grid point g = 2^e (1 + i 2^-24), degree n
static inline void pair(long long n, double dn, int e, uint32_t i) {
  if (e == 0 && i == 0) return;  // g = 1: only x = 1, handled exactly by the library
  npairs++;
  double lh, ll;  // log2 g
  dd_fast_two_sum((double) e, Th[i], &lh, &ll);
  ll += Tl[i];
  double th = dn * lh;
  double tl = fma(dn, lh, -th) + dn * ll;
  dd_fast_two_sum(th, tl, &th, &tl);
  if (th < -151.0 || th >= 128.0) return;
  double et = floor(th);
  if (th == et && tl < 0) et -= 1.0;
  double fh, fl;  // f = t - et in [0, 1)
  dd_two_sum(th - et, tl, &fh, &fl);
  double rh, rl;
  uint32_t xb;
  if (et >= -126.0) {
    double m = exp2(fh);  // in [1, 2]; only a starting guess
    // NaN / out-of-range guesses are clamped before any integer conversion
    int64_t ix = !(m > 1.0) ? 0 : m >= 2.0 ? (1 << 23) : (int64_t) rint((m - 1.0) * 0x1p23);
    // libm-free correction: the nearest mantissa 1 + ix 2^-23 is the one whose
    // rounding cell [log2 of lower midpoint, log2 of upper midpoint) contains f;
    // midpoint logs are the odd table entries Th[2 ix +- 1] + Tl[...].
    while (ix > 0 && below(fh, fl, Th[2 * ix - 1], Tl[2 * ix - 1])) ix--;
    while (ix < (1 << 23) && !below(fh, fl, Th[2 * ix + 1], Tl[2 * ix + 1])) ix++;
    if (ix >= (1 << 23)) {  // nearest binary32 is 2^(et+1)
      if (et + 1.0 > 127.0) return;
      dd_two_sum(fh - 1.0, fl, &rh, &rl);
      xb = (uint32_t) (et + 128.0) << 23;
    } else {
      if (ix < 0) ix = 0;
      dd_two_sum(fh - Th[2 * ix], fl - Tl[2 * ix], &rh, &rl);
      xb = ((uint32_t) (et + 127.0) << 23) | (uint32_t) ix;
    }
  } else {
    // X = 2^t < 2^-126: nearest binary32 is mx 2^-149 with mx integer
    double sh, sl;  // s = t + 149
    dd_two_sum(th + 149.0, tl, &sh, &sl);  // th + 149 exact (|th| < 2^8)
    double mx = rint(exp2(sh));  // only a starting guess
    // libm-free correction against log2(mx +- 1/2) = log2(2 mx +- 1) - 1
    if (!(mx >= 0.0)) mx = 0.0;  // also catches NaN
    if (mx > 0x1p23) mx = 0x1p23;
    for (;;) {
      double ah, al;
      if (mx >= 1.0) {
        log2_int((uint32_t) (2.0 * mx - 1.0), &ah, &al);
        if (below(sh, sl, ah - 1.0, al)) { mx -= 1.0; continue; }
      }
      if (mx < 0x1p23) {
        log2_int((uint32_t) (2.0 * mx + 1.0), &ah, &al);
        if (!below(sh, sl, ah - 1.0, al)) { mx += 1.0; continue; }
      }
      break;
    }
    if (mx < 1.0) return;
    double lh2, ll2;
    log2_int((uint32_t) mx, &lh2, &ll2);
    dd_two_sum(sh - lh2, sl - ll2, &rh, &rl);
    xb = (uint32_t) mx;  // subnormal (or 2^-126) bit pattern
  }
  double r = rh + rl;
  if (fabs(r) < thr) report(n, xb, e, i, r);
}

// first i in [0, 2^24) with e + Th[i] >= v (2^24 if none)
static uint32_t lower_i(int e, double v) {
  uint32_t lo = 0, hi = 1u << 24;
  while (lo < hi) {
    uint32_t mid = lo + (hi - lo) / 2;
    if ((double) e + Th[mid] >= v) hi = mid; else lo = mid + 1;
  }
  return lo;
}

int main(int argc, char **argv) {
  if (argc < 5) { fprintf(stderr, "usage: dualscan n0 n1 Rlog2 out\n"); return 2; }
  long long n0 = atoll(argv[1]), n1 = atoll(argv[2]);
  thr = ldexp(1.0, atoi(argv[3]));
  out = fopen(argv[4], "a");
  setvbuf(out, NULL, _IOLBF, 0);
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  build_table();
  clock_gettime(CLOCK_MONOTONIC, &t1);
  fprintf(stderr, "table built in %.1f s\n",
          (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec));
  uint64_t tot_pairs = 0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (long long n = n0; n <= n1; n++) {
    if (n > -2 && n < 2) continue;
    double dn = (double) n;
    // t = n log2 g must lie in [-151, 128): range of log2 g (with slack)
    double a = -151.0 / dn, b = 128.0 / dn;
    double lo = fmin(a, b) - 0x1p-30, hi = fmax(a, b) + 0x1p-30;
    ncand = npairs = 0;
    for (int e = (int) floor(lo); e <= (int) floor(hi); e++) {
      uint32_t ilo = lower_i(e, lo), ihi = lower_i(e, hi);  // [ilo, ihi)
      if (ihi < (1u << 24) && (double) e + Th[ihi] <= hi) ihi++;
      for (uint32_t i = ilo; i < ihi; i++) pair(n, dn, e, i);
    }
    tot_pairs += npairs;
    if (n1 - n0 < 64) fprintf(stderr, "n=%lld pairs=%llu cand=%llu\n", n,
                              (unsigned long long) npairs, (unsigned long long) ncand);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  double sec = (t1.tv_sec - t0.tv_sec) + 1e-9 * (t1.tv_nsec - t0.tv_nsec);
  printf("n=[%lld,%lld] pairs=%llu sec=%.2f ns/pair=%.2f\n", n0, n1,
         (unsigned long long) tot_pairs, sec, sec * 1e9 / (double) (tot_pairs ? tot_pairs : 1));
  fclose(out);
  return 0;
}
