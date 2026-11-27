/* Independent driver: read "xbits_hex n" lines, call cr_rootnf in all four
   rounding modes, print result bits, raised flags and errno for each mode. */
#include <errno.h>
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rootnf.h"

static const int modes[4] = {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};

int main(void) {
  char line[256];
  while (fgets(line, sizeof line, stdin)) {
    unsigned xb;
    long long n;
    if (sscanf(line, "%x %lld", &xb, &n) != 2) {
      fprintf(stderr, "bad line: %s", line);
      return 1;
    }
    float x;
    memcpy(&x, &xb, 4);
    printf("%08x %lld", xb, n);
    for (int m = 0; m < 4; m++) {
      fesetround(modes[m]);
      feclearexcept(FE_ALL_EXCEPT);
      errno = 0;
      volatile float r = cr_rootnf(x, n);
      int fl = fetestexcept(FE_ALL_EXCEPT);
      int e = errno;
      fesetround(FE_TONEAREST);
      float rr = r;
      uint32_t rb;
      memcpy(&rb, &rr, 4);
      int mask = ((fl & FE_INEXACT) ? 1 : 0) | ((fl & FE_UNDERFLOW) ? 2 : 0) |
                 ((fl & FE_OVERFLOW) ? 4 : 0) | ((fl & FE_DIVBYZERO) ? 8 : 0) |
                 ((fl & FE_INVALID) ? 16 : 0);
      printf(" %08x %d %d", rb, mask, e);
    }
    putchar('\n');
  }
  return 0;
}
