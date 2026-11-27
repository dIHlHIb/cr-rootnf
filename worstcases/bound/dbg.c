// Inspect the normal, no-carry case using the initial exp2 estimate before midpoint correction.
#include <stdio.h>
#include <stdlib.h>
#include "ddlib.h"
int main(int argc, char **argv) {
  if (argc != 4) return 2;
  long long n = atoll(argv[1]);
  int e = atoi(argv[2]);
  uint32_t i = (uint32_t) atol(argv[3]);
  double dn = (double) n, Thi, Tli;
  dd_log2(1.0 + (double) i * 0x1p-24, &Thi, &Tli);
  double lh, ll;
  dd_fast_two_sum((double) e, Thi, &lh, &ll);
  printf("Th=%a Tl=%a lh=%a ll0=%a\n", Thi, Tli, lh, ll);
  ll += Tli;
  double th = dn * lh;
  double fm = fma(dn, lh, -th), nl = dn * ll;
  double tl = fm + nl;
  printf("th=%a fma=%a n*ll=%a tl=%a\n", th, fm, nl, tl);
  dd_fast_two_sum(th, tl, &th, &tl);
  double et = floor(th);
  if (th == et && tl < 0) et -= 1.0;
  if (!(et >= -126 && et <= 127)) return 2;
  double fh, fl;
  dd_two_sum(th - et, tl, &fh, &fl);
  double m = exp2(fh);
  if (!(m >= 1.0 && m < 2.0)) return 2;
  int64_t ix = (int64_t) rint((m - 1.0) * 0x1p23);
  if (ix >= (1 << 23)) return 2;
  double T2h, T2l;
  dd_log2(1.0 + (double) (2 * ix) * 0x1p-24, &T2h, &T2l);
  double rh, rl;
  dd_two_sum(fh - T2h, fl - T2l, &rh, &rl);
  printf("t=%a+%a et=%g fh=%a fl=%a ix=%lld xb=%08x r=%a\n", th, tl, et, fh, fl, (long long) ix,
         ((uint32_t) (et + 127.0) << 23) | (uint32_t) ix, rh + rl);
  return 0;
}
