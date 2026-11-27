"""Write fpe.c: the fast path of cr_rootnf, cut verbatim from rootnf.c.

The exact-case shortcut is dropped so that every input reaches the
approximation; the driver prints the binary64 value y before the guard
test, in all four rounding modes.
Usage: python3 make_fpe.py path/to/rootnf.c > fpe.c
"""
import sys

src = open(sys.argv[1]).read()
start = src.index("    int e = (int)(mag >> 23) - 127;")
end = src.index("    double y = from_bits(uy);") + len("    double y = from_bits(uy);")
body = src[start:end]
ex0 = body.index("    if (!mant || (n >= 3 && n <= 15)) {")
ex1 = body.index("    double reciprocal")
body = body[:ex0] + body[ex1:]
sys.stdout.write('''#include "rootnf.c"
#include <stdio.h>
static double fast_y(float x, long long n) {
    uint32_t bits;
    memcpy(&bits, &x, sizeof(bits));
    uint32_t sign = bits & 0x80000000u, mag = bits ^ sign;
    (void)sign;
%s
    return y;
}
static const int modes[4] = {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
int main(void) {
    char line[128];
    while (fgets(line, sizeof line, stdin)) {
        unsigned xb; long long n;
        if (sscanf(line, "%%x %%lld", &xb, &n) != 2) return 1;
        float x; memcpy(&x, &xb, 4);
        printf("%%08x %%lld", xb, n);
        for (int m = 0; m < 4; m++) {
            fesetround(modes[m]);
            volatile double y = fast_y(x, n);
            fesetround(FE_TONEAREST);
            double yy = y; uint64_t u; memcpy(&u, &yy, 8);
            printf(" %%016llx", (unsigned long long)u);
        }
        putchar('\\n');
    }
    return 0;
}
''' % body)
