"""Measured |r_scanner - r| over all candidates, and libm exp2 relative error."""
import ctypes
import math
import random
import struct
import sys
import glob
import gmpy2
from gmpy2 import mpfr
gmpy2.get_context().precision = 300
worst = mpfr(0)
where = None
cnt = 0
for fn in glob.glob(sys.argv[1] + "/*.txt"):
    for line in open(fn):
        n, xh, e, i, rh = line.split()
        n, e, i = int(n), int(e), int(i)
        x = struct.unpack(">f", bytes.fromhex(xh))[0]
        g = mpfr(2) ** e * (1 + mpfr(i) * mpfr(2) ** -24)
        r = gmpy2.log2(mpfr(x)) - n * gmpy2.log2(g)
        d = abs(r + mpfr(float.fromhex(rh)))  # scanner reports n log2 g - log2 x
        cnt += 1
        if d > worst:
            worst, where = d, (n, xh, e, i)
print("candidates %d, max |r_scan - r| = 2^%.2f at %s" % (cnt, float(gmpy2.log2(worst)), where))
libm = ctypes.CDLL(None)
libm.exp2.restype = ctypes.c_double
libm.exp2.argtypes = [ctypes.c_double]
rng = random.Random(1)
w = 0.0
for k in range(1000000):
    v = rng.uniform(0, 1) if k % 2 else rng.uniform(-2, 23)
    ex = gmpy2.exp2(mpfr(v))
    w = max(w, float(abs(mpfr(libm.exp2(v)) - ex) / ex))
print("libm exp2 max relative error on 1e6 points = 2^%.2f" % math.log2(w))
